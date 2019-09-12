SRC_BASE_PATH=/root/seastar/phxpaxos

all:lib_phxkv_client phxkv_client_tools_bin phxkv_grpcserver_bin 

include $(SRC_BASE_PATH)/makefile.mk

include Makefile.define
PHXKV_CLIENT_SRC=$(PHXKV_CLIENT_OBJ)
PHXKV_CLIENT_INCS=$(sort $(SRC_BASE_PATH)/sample/phxkv $(GRPC_INCLUDE_PATH) $(PHXPAXOS_INCLUDE_PATH) $(LEVELDB_INCLUDE_PATH) $(PROTOBUF_INCLUDE_PATH))
PHXKV_CLIENT_FULL_LIB_PATH=$(SRC_BASE_PATH)//sample/phxkv
PHXKV_CLIENT_EXTRA_CPPFLAGS=-Wall

CPPFLAGS+=$(patsubst %,-I%, $(PHXKV_CLIENT_INCS))
CPPFLAGS+=$(PHXKV_CLIENT_EXTRA_CPPFLAGS)

lib_phxkv_client: phxkv_client_dir $(SRC_BASE_PATH)/.lib/libphxkv_client.a $(SRC_BASE_PATH)/.lib/extlib/libphxkv_client.a

phxkv_client_dir:$(PHXKV_CLIENT_FULL_LIB_PATH)
	@for dir in $^;\
	do \
	current_dir=`readlink $$dir -m`;\
	pwd_dir=`pwd`;\
	pwd_dir=`readlink $$pwd_dir -m`;\
	if ([ "$$current_dir" != "$$pwd_dir" ]); then \
	make -C $$dir;\
	fi;\
	done

$(SRC_BASE_PATH)/.lib/libphxkv_client.a: $(PHXKV_CLIENT_SRC)
	ar -cvq $@ $(PHXKV_CLIENT_SRC)

PHXKV_CLIENT_LIB_OBJ=$(patsubst %, $(SRC_BASE_PATH)/%, /sample/phxkv/phxkv.pb.o /sample/phxkv/phxkv.grpc.pb.o /sample/phxkv/kv_grpc_client.o /sample/phxkv/kv_grpc_client_main.o)
$(SRC_BASE_PATH)/.lib/extlib/libphxkv_client.a: $(PHXKV_CLIENT_LIB_OBJ)
	ar -cvq $@ $(PHXKV_CLIENT_LIB_OBJ)

PHXKV_CLIENT_TOOLS_SRC=$(PHXKV_CLIENT_TOOLS_OBJ)
PHXKV_CLIENT_TOOLS_INCS=$(sort $(GRPC_INCLUDE_PATH) $(PHXPAXOS_INCLUDE_PATH) $(LEVELDB_INCLUDE_PATH) $(PROTOBUF_INCLUDE_PATH) $(SRC_BASE_PATH)/sample/phxkv)
PHXKV_CLIENT_TOOLS_FULL_LIB_PATH=$(SRC_BASE_PATH)/sample/phxkv $(SRC_BASE_PATH)//sample/phxkv
PHXKV_CLIENT_TOOLS_EXTRA_CPPFLAGS=-Wall

CPPFLAGS+=$(patsubst %,-I%, $(PHXKV_CLIENT_TOOLS_INCS))
CPPFLAGS+=$(PHXKV_CLIENT_TOOLS_EXTRA_CPPFLAGS)

PHXKV_CLIENT_TOOLS_LINK=phxkv_client
PHXKV_CLIENT_TOOLS_SYS_LIB=$(GRPC_LIBE_PATH)/libgrpc++_unsecure.a $(GRPC_LIBE_PATH)/libgrpc.a -lssl -lcrypto -ldl -lz -lrt $(PHXPAXOS_LIB_PATH)/libphxpaxos.a $(LEVELDB_LIB_PATH)/libleveldb.a $(PROTOBUF_LIB_PATH)/libprotobuf.a -lpthread
PHXKV_CLIENT_TOOLS_FLAGS+=$(LDFLAGS)

PHXKV_CLIENT_TOOLS_FLAGS+=$(patsubst %,-l%, $(PHXKV_CLIENT_TOOLS_LINK))
PHXKV_CLIENT_TOOLS_FLAGS+=$(PHXKV_CLIENT_TOOLS_SYS_LIB)
phxkv_client_tools_bin:phxkv_client_tools_dir phxkv_client_tools

phxkv_client_tools_dir:$(PHXKV_CLIENT_TOOLS_FULL_LIB_PATH)
	@for dir in $^;\
	do \
	current_dir=`readlink $$dir -m`;\
	pwd_dir=`pwd`;\
	pwd_dir=`readlink $$pwd_dir -m`;\
	if ([ "$$current_dir" != "$$pwd_dir" ]); then \
	make -C $$dir;\
	fi;\
	done

phxkv_client_tools:$(PHXKV_CLIENT_TOOLS_SRC)
	$(CXX) $^ -o $@ $(PHXKV_CLIENT_TOOLS_FLAGS)
	cp $@ $(SRC_BASE_PATH)/.sbin/

PHXKV_GRPCSERVER_SRC=$(PHXKV_GRPCSERVER_OBJ)
PHXKV_GRPCSERVER_INCS=$(sort $(SRC_BASE_PATH)/sample/phxkv $(PHXPAXOS_PLUGIN_PATH) $(GRPC_INCLUDE_PATH) $(PHXPAXOS_INCLUDE_PATH) $(LEVELDB_INCLUDE_PATH) $(PROTOBUF_INCLUDE_PATH))
PHXKV_GRPCSERVER_FULL_LIB_PATH=$(SRC_BASE_PATH)//sample/phxkv
PHXKV_GRPCSERVER_EXTRA_CPPFLAGS=-Wall

CPPFLAGS+=$(patsubst %,-I%, $(PHXKV_GRPCSERVER_INCS))
CPPFLAGS+=$(PHXKV_GRPCSERVER_EXTRA_CPPFLAGS)

PHXKV_GRPCSERVER_LINK=
PHXKV_GRPCSERVER_SYS_LIB=$(PHXPAXOS_LIB_PATH)/libphxpaxos_plugin.a $(PHXPAXOS_LIB_PATH)/libphxpaxos.a $(LEVELDB_LIB_PATH)/libleveldb.a $(GRPC_LIBE_PATH)/libgrpc++_unsecure.a $(GRPC_LIBE_PATH)/libgrpc.a $(PROTOBUF_LIB_PATH)/libprotobuf.a $(GLOG_LIB_PATH)/libglog.a $(GFLAGS_LIB_PATH)/libgflags.a -lssl -lcrypto -ldl -lz -lrt -lpthread
PHXKV_GRPCSERVER_FLAGS+=$(LDFLAGS)

PHXKV_GRPCSERVER_FLAGS+=$(patsubst %,-l%, $(PHXKV_GRPCSERVER_LINK))
PHXKV_GRPCSERVER_FLAGS+=$(PHXKV_GRPCSERVER_SYS_LIB)
phxkv_grpcserver_bin:phxkv_grpcserver_dir phxkv_grpcserver

phxkv_grpcserver_dir:$(PHXKV_GRPCSERVER_FULL_LIB_PATH)
	@for dir in $^;\
	do \
	current_dir=`readlink $$dir -m`;\
	pwd_dir=`pwd`;\
	pwd_dir=`readlink $$pwd_dir -m`;\
	if ([ "$$current_dir" != "$$pwd_dir" ]); then \
	make -C $$dir;\
	fi;\
	done

phxkv_grpcserver:$(PHXKV_GRPCSERVER_SRC)
	$(CXX) $^ -o $@ $(PHXKV_GRPCSERVER_FLAGS)
	cp $@ $(SRC_BASE_PATH)/.sbin/

clean:
	rm -rf *.o *.pb.* $(SRC_BASE_PATH)/.lib/libphxkv_client.a $(SRC_BASE_PATH)/.lib/extlib/libphxkv_client.a phxkv_client_tools $(SRC_BASE_PATH)/.sbin/phxkv_client_tools phxkv_grpcserver $(SRC_BASE_PATH)/.sbin/phxkv_grpcserver libphxkv_client.a $(SRC_BASE_PATH)/.lib/libphxkv_client.a phxkv_client_tools phxkv_grpcserver 

