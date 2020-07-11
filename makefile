all:	tests bbcounttool btracetool ctcounttool mallocwraptool maxstacktool

clean: clean_tests clean_bbcounttool clean_btracetool clean_ctcounttool clean_mallocwraptool clean_maxstacktool

tests:
	(cd Tests && make all && cd ..)

bbcounttool:
	(cd BBCountTool && chmod +x bbcount && make && cd ..)

btracetool:
	(cd BtraceTool && chmod +x btrace  && make && cd ..)

ctcounttool:
	(cd CTCountTool && chmod +x ctcount && make && cd ..)

mallocwraptool:
	(cd MallocWrapTool && chmod +x wrapmalloc  && make && cd ..)

maxstacktool:
	(cd MaxStackTool && chmod +x maxstack && make && cd ..)

clean_tests:
	(cd Tests && rm *.out && cd ..)

clean_bbcounttool:
	rm -rf BBCountTool/obj-ia32/

clean_btracetool:
	rm -rf BtraceTool/obj-ia32/

clean_ctcounttool:
	rm -rf CTCountTool/obj-ia32/

clean_mallocwraptool:
	rm -rf MallocWrapTool/obj-ia32/

clean_maxstacktool:
	rm -rf MaxStackTool/obj-ia32/
