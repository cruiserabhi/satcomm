##############################################################################
#  Copyright (c) 2017, The Linux Foundation. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
#  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##############################################################################

export TELUX_GIT_ROOT=$(shell git rev-parse --show-toplevel)
export TITLE_PAGE=$(TELUX_GIT_ROOT)/doxy/api_reference/title_page.dox

all: html

html: logs
	# run the html output
	ALIAS_INCLUDE_FILE=../template/other_html_aliases.txt \
	ENV_WARN_LOG=logs/api_reference_html_doxy_warn.log \
	ENV_GEN_HTML=YES \
	ENV_GEN_LATEX=NO \
	ENV_LAYOUT_FILE=Doxygen_Layout_Standard_UserManual_html.xml \
	doxygen Doxy_Config.ldd > logs/api_reference_html_doxy.log
	# copy images into html folder
	cp -r $(TELUX_GIT_ROOT)/doxy/images/ $(TELUX_GIT_ROOT)/doxy/api_reference/html/images/

logs:
	mkdir -p logs

clean:
	rm -rf logs
	rm -rf html
	rm -rf images
