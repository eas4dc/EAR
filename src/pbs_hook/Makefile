all: libeard_rapi.so libhardware.so

libeard_rapi.so:
	gcc --shared -o libeard_rapi.so ../daemon/remote_api/eard_rapi.o -L../common -lcommon

libhardware.so:
	gcc --shared -o libhardware.so ../common/hardware/*.o -L../common -lcommon

clean:
	rm *.so
