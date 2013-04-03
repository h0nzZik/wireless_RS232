name="coordinator"
coord:
	mkdir -p out/coord
	cc5x ${name}.c -Oout/coord

node:
	mkdir -p out/node
	cc5x node.c -Oout/node
install:
	iqrf-programmer -i out/coord/coordinator.hex
install_node:
	iqrf-programmer -i out/node/node.hex

