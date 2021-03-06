# @par
# IXP400 SW Release Crypto version 2.3
# 
# -- Copyright Notice --
# 
# @par
# Copyright (c) 2001-2005, Intel Corporation.
# All rights reserved.
# 
# @par
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the Intel Corporation nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# 
# @par
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 
# 
# @par
# -- End of Copyright Notice --

cryptoAcc_OBJ := IxCryptoAccUtilities.o		\
		 IxCryptoAcc.o			        \
		 IxCryptoAccDescMgmt.o		    \
		 IxCryptoAccCCDMgmt.o		    \
		 IxCryptoAccQAccess.o		    \
		 IxCryptoAccCryptoRegister.o	\
		 IxCryptoAccCryptoPerform.o
		 
ifneq ($(IX_DEVICE),ixp42X)
cryptoAcc_OBJ += IxCryptoAccPkeCommon.o     \
		 IxCryptoAccPkeEau.o                \
		 IxCryptoAccPkeHash.o
endif

# Exclude assembly code from unit test
# Use the stub in unit test folder
ifneq ($(IX_TARGET),vxsim)
cryptoAcc_OBJ += IxCryptoAccXScaleWep.o
endif

cryptoAcc_CFLAGS := -I./src/cryptoAcc/include -I./$(LINUX_SRC)/include/asm-arm

# Force NDEBUG unless debug info has been specifically requested.
# This way the unit test will be built with NDEBUG.
ifndef IX_DEBUG
cryptoAcc_CFLAGS += -DNDEBUG
endif

ifeq ($(IX_TARGET_OS),linux)
cryptoAcc_OBJ += IxCryptoAccSymbols.o
cryptoAcc_EXPORT_OBJ := IxCryptoAccSymbols.o
endif
