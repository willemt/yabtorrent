letsdothis:
	if test -e waf; \
	then python waf ; \
	else echo Downloading waf...; wget http://waf.googlecode.com/files/waf-1.7.2; mv waf-1.7.2 waf; python waf configure; python waf; \
	fi
