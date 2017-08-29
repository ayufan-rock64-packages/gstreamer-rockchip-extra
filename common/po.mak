# rule to download the latest .po files
download-po: $(top_srcdir)/common/download-translations
	$(top_srcdir)/common/download-translations $(PACKAGE)
	for f in po/*.po; do \
	  num_changed=`git diff $$f | grep -e '^[+-][^+-]' | wc -l`; \
	  num_date=`git diff $$f | grep -e '^[+-][^+-]' | grep POT-Creation-Date | wc -l`; \
	  if [ $num_date == $num_changed ]; then \
	    git checkout $$f; \
	  fi; \
	done

