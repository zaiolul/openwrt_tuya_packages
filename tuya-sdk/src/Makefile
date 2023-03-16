SDKBUILD:=./build

.PHONY: all clean sdk 

all: sdk 

sdk: 
	mkdir $(SDKBUILD)
	cmake -S$(CURDIR) -B$(SDKBUILD)
	$(MAKE) -C $(SDKBUILD)

clean:
	rm -rf $(SDKBUILD)

