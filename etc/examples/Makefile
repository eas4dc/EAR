EAR_SRC=/home/bsc/bsc019620/wf_support/ear_private/src
BIN=ear_api

all:$(BIN)

ear_api:ear_api.c
	$(CC) $(CFLAGS) -I$(EAR_INSTALL_PATH)/include -o ear_api ear_api.c -L$(EAR_INSTALL_PATH)/lib -lear_api  $(EAR_SRC)/common/libcommon.a

clean:
	rm $(BIN)

