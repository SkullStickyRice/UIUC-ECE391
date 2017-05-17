# interrupt linkage
.text
# kernal to user level linkages for keyboard and rtc
.global keyboard_linkage, rtc_linkage, pit_linkage

keyboard_linkage:
	pushfl
	pushal
	call keyboard_handler
	popal
	popfl

	iret

rtc_linkage:
	pushfl
	pushal
	call rtc_handler
	popal 
	popfl

	iret

pit_linkage:
	pushfl
	pushal
	call pit_handler
	popal 
	popfl

	iret
