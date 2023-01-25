# :dog2: 

This is a side template-project, where i implement a simple distributed file system on ESP32 nodes using their FLASH memory to store plain text files.
The networking is done over simple TCP implementation, where nodes connect to a server that keeps things working.
Following commands (provided on the server console) are available:

- WRITE,FILENAME,DATA_PLAIN_TEXT

Writes (creates a new file)

- APPEND,FILENAME,DATA_PLAIN_TEXT

Appends to a file

- DELETE,FILENAME

Deletes the file

- LS

Lists available files and replicates across nodes if hashes differ

- FORMAT

Performs SPIFFS formatting
 
# Demo

Lets simply run the application and play with the filesystem.


# Prepare server and clients

We will compile and run the server, then upload the sketch and run the DFS nodes.


0. Compile server

`./build.sh`

1. Start server

`./a.out`

2. Upload Sketch to ESP32 Nodes

Ensure to update following definitions to match your setup.

- `WIFI_SSID`
- `WIFI_PWD`
- `DFS_SERVER`

3. Wait for nodes to connect to server

`192.168.1.136:60584 Connected`

`192.168.1.115:61518 Connected`

`192.168.1.115:53053 Connected`


# Play with FS

1. Create a file

`WRITE,github,HELLO_WORLD`

`Writing to file 'github' 11 bytes of data`

Will create a github file, with a "HELLO WORLD" inside, storing it on available (connected) nodes.

2. List created files

We will list files created and see where are they stored together with their MD5 hash compared to the master version on the server.

`LS`
 
`- github (11b) eba8edba90d6aabf1435c5843586682e`

`-- 192.168.1.115@eba8edba90d6aabf1435c5843586682e (Checksum OK)`

`-- 192.168.1.128@eba8edba90d6aabf1435c5843586682e (Checksum OK)`

`-- 192.168.1.136@eba8edba90d6aabf1435c5843586682e (Checksum OK)`

3. Append a file

`APPEND,github,__HELLO__ALL__`

`Writing to file 'github' 14 bytes of data`

4. Delete a file

`DELETE,github`

`Deleting file 'github'`
   

# Important

This is nowhere near to be completed, it was made for fun to run on local (private) network - but serves as a good base for expansion.
I tried keeping things easy so hopefully if you are interested you can tweak and expand it further.

# Next

Some further updates can be made and here are some ideas to keep you going.

- Allow TCP clients to use the files over the network
- Implement better replication (maybe chunks)
- Sync with nodes that are connecting instead of waiting for `ls` 
- Implement bytes writing, reading (instead of plain text) 

