To build executable run: gcc -pthread -lcurl -o digest_auth_upload digest_auth_upload.c

Executable need following Input parameter:
 digest_auth_upload "watchFolder full path" "url" "user:pwd"

1- "watchFolder full path": Application will monitor this folder and when any new file comes, then the application will start HTTP push with digest authentication.
NOTE: watchFolder full path ending with /
Example: /home/abhisheks/Taha/HLS/upload/

2- "url": AWS Input Channel path of MediaPackager where we have to do HTTP push.
NOTE: AWS Input url excluding channel. Example:
For MediaPackager input channel url: https://8dc923984a2db229.mediapackage.ap-south-1.amazonaws.com/in/v2/a3ff03c87d5643bd876412cb3d44599f/a3ff03c87d5643bd876412cb3d44599f/channel
, the url for our application parameter will be:
https://8dc923984a2db229.mediapackage.ap-south-1.amazonaws.com/in/v2/a3ff03c87d5643bd876412cb3d44599f/a3ff03c87d5643bd876412cb3d44599f/

3- "user:pwd": Digest authentication credentials of MediaPackager Input Channel.


NOTE: Before running the application make sure watchFolder is empty.