INITRD := initrd.tar
SRC := src
OUT := out

.PHONY: all
all: $(INITRD)

$(INITRD): $(OUT)/user
	tar --format=ustar -C $(OUT)/ -cf $@ .

$(OUT)/user: $(SRC)/user.s
	mkdir -p "$(dir $@)"
	gcc $< -o $@ -nostdlib -static

.PHONY: clean
clean:
	rm -rf $(INITRD) $(OUT)/
