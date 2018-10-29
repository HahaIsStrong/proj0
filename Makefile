X86:server.c
	@rm -rf _X86
	@mkdir _X86
	gcc server.c -o _X86/server -lsqlite3 -lpthread
	cp ./mydb.db _X86/
	gcc register.c -o _X86/register
	gcc login.c -o _X86/login
	gcc logout.c -o _X86/logout
	gcc client_cmd.c -o _X86/client_cmd 
	gcc shm_opt.c -o _X86/shm_opt
 



arm:server.c
	arm-none-linux-gnueabi-gcc server.c -o server -lsqlite3 -L/home/linux/sqlite-arm/lib -I/home/linux/sqlite-arm/include -lpthread 
	sudo cp ./server /rootfs
	sudo cp ./mydb.db /rootfs	
	sudo touch /rootfs/tmpfile
	sudo chmod 777 /rootfs/tmpfile 
	echo "******************************  编译之前 , 请确认头文件中MYFILE为 根目录下的tmpfile"

s:
	touch server.c


