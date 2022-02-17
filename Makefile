
all:
	for d in db util; do make -j $$MAKE_JOBS -C $$d; done
	@for d in *; do test "$$d" = "old" && continue; if test -d $$d; then make -j $$MAKE_JOBS -C $$d || exit 1; fi; done

clients:
	for d in db util; do make -j $$MAKE_JOBS -C $$d; done
	@for d in *; do test -f $$d/.client && make -j $$MAKE_JOBS -C $$d install; done

gendb::
	sudo mkdir -p /var/www/html/tools/
	@for d in util db; do make -j $$MAKE_JOBS -C $$d DEBUG=no clean; done
	@for d in util db; do make -j $$MAKE_JOBS -C $$d DEBUG=no; done
	@for d in gendb; do make -j $$MAKE_JOBS -C $$d DEBUG=no clean; done
	@for d in gendb; do make -j $$MAKE_JOBS -C $$d DEBUG=no install; done

install clean cleanall::
	for d in *; do if test -d $$d; then make -C $$d $@; fi; done

list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$'
