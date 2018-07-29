# DROPBOX_LIKE
A simple dropbox-like network application.

### [I].The concept of this homework is like dropbox.
  First, an user can save his files on the server.
  Second, the clients of the user are running on different hosts.
  Third, when any of client of the user upload a file, the server have to transmit it to all the other clients of the user.
  Forth, when a new client of the user connects to the server, the server should transmit all the files, which are uploaded by the other clients of the user, to the new client immediately.
  Fifth, we will type /put fileXXX on different clients at the same time, your programs have to deal with this case. 
  Sixth, if one of the client is sleeping, server has to send the data to the other clients of the user in a non-blocking way.
  Seventh, the uploading data only need to send to the clients which are belong to the user.

Note: In your server program, you should include these two lines to achieve the non-blocking function.
   "int flag=fcntl(sock,F_GETFL,0);"
   "fcnctl(sock,F_SETTFL,flag|O_NONBLOCK);"
   Please confirm that your server is non-blocking, we will test the correctness of your server when the server's sending buffer is full.

### [II].Inputs
   1. ./client <ip> <port> <username>
	  Please make sure that the you should excute the client program in this format.

   2. ./server <port>
      Please make sure that the you should excute the server program in this format.

   3. /put <filename>
      This command, which is used on client side by users, is to upload your files to the server side.
      Users can transmit any files they want, but it has to be put in the same directory of client.
      The filename should send to the other clients.
      Client put the file which it received in the same directory of client.

   4. /sleep <seconds>
      This command is to let client to sleep.

   5. /exit
       This command is to disconnect with server and terminate the program.


### [III].Outputs
   1. Welcome message. 
      "Welcome to the dropbox-like server! : <username>" 
   2. Uploading progess bar.
	Downloading file : <filename>
	Progress : [############                      ]
	Download <filename> complete!
   3. Downloading progess bar.
	Uploading file : <filename>
	Progress : [######################]
	Upload <filename> complete!
   5. Sleeping count down.
	/sleep 20
	Client starts to sleep
	Sleep 1
	.
	.
	Sleep 19
	Sleep 20
	Client wakes up    


