#ifndef __LINUX_NET_SCM_H
#define __LINUX_NET_SCM_H

#include <linux/limits.h>
#include <linux/net.h>

/* Well, we should have at least one descriptor open
 * to accept passed FDs 8)
 */
#define SCM_MAX_FD	(OPEN_MAX-1)

struct scm_fp_list
{
	int		count;
	struct file	*fp[SCM_MAX_FD];
};

struct scm_cookie
{
	struct ucred		creds;		/* Skb credentials	*/
	struct scm_fp_list	*fp;		/* Passed files		*/
#ifdef CONFIG_SECURITY_NETWORK
	char			*secdata;	/* Security context	*/
	u32			seclen;		/* Security length	*/
#endif
	unsigned long		seq;		/* Connection seqno	*/
};

extern void scm_detach_fds(struct msghdr *msg, struct scm_cookie *scm);
extern void scm_detach_fds_compat(struct msghdr *msg, struct scm_cookie *scm);
extern int __scm_send(struct socket *sock, struct msghdr *msg, struct scm_cookie *scm);
extern void __scm_destroy(struct scm_cookie *scm);
extern struct scm_fp_list * scm_fp_dup(struct scm_fp_list *fpl);

static __inline__ void scm_destroy(struct scm_cookie *scm)
{
	if (scm && scm->fp)
		__scm_destroy(scm);
}

static __inline__ int scm_send(struct socket *sock, struct msghdr *msg,
			       struct scm_cookie *scm)
{
	struct task_struct *p = current;
	scm->creds.uid = p->uid;
	scm->creds.gid = p->gid;
	scm->creds.pid = p->tgid;
	scm->fp = NULL;
	scm->seq = 0;
	if (msg->msg_controllen <= 0)
		return 0;
	return __scm_send(sock, msg, scm);
}

#ifdef CONFIG_SECURITY_NETWORK
static inline void scm_passec(struct socket *sock, struct msghdr *msg, struct scm_cookie *scm)
{
	if (test_bit(SOCK_PASSSEC, &sock->flags) && scm->secdata != NULL)
		put_cmsg(msg, SOL_SOCKET, SCM_SECURITY, scm->seclen, scm->secdata);
}
#else
static inline void scm_passec(struct socket *sock, struct msghdr *msg, struct scm_cookie *scm)
{ }
#endif /* CONFIG_SECURITY_NETWORK */

static __inline__ void scm_recv(struct socket *sock, struct msghdr *msg,
				struct scm_cookie *scm, int flags)
{
	if (!msg->msg_control)
	{
		if (test_bit(SOCK_PASSCRED, &sock->flags) || scm->fp)
			msg->msg_flags |= MSG_CTRUNC;
		scm_destroy(scm);
		return;
	}

	if (test_bit(SOCK_PASSCRED, &sock->flags))
		put_cmsg(msg, SOL_SOCKET, SCM_CREDENTIALS, sizeof(scm->creds), &scm->creds);

	scm_passec(sock, msg, scm);

	if (!scm->fp)
		return;
	
	scm_detach_fds(msg, scm);
}


#endif /* __LINUX_NET_SCM_H */

