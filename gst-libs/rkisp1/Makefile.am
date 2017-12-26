LIB_RK_AIQ=librk_aiq.so

all:
	cp $(LIB_RK_AIQ).$(target_cpu) $(LIB_RK_AIQ)

install-data-am:
	echo " $(MKDIR_P) '$(DESTDIR)$(libdir)'"; \
	$(MKDIR_P) "$(DESTDIR)$(libdir)" || exit 1; \
	install -m 0644 $(LIB_RK_AIQ) $(DESTDIR)$(libdir)/$(LIB_RK_AIQ)
