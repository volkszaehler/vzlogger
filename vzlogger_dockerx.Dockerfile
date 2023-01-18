# Dockerfile (dockerx) to create an arm docker image using the buildroot-based builder
ARG	BUILDER
FROM	--platform=$BUILDPLATFORM $BUILDER as builder

RUN \
	set -xe ; \
	./br_make_wrapper vzlogger-build ;\
	file output/build/vzlogger-*/src/vzlogger ; \
	ls -l output/build/vzlogger-*/src/vzlogger ; \
	file output/build/vzlogger-origin_master/tests/vzlogger_unit_tests ; \
	ls -l output/build/vzlogger-origin_master/tests/vzlogger_unit_tests
RUN \
	./br_make_wrapper vzlogger-install-target

FROM scratch as arm-image
COPY --from=builder /buildroot/output/target/ /

