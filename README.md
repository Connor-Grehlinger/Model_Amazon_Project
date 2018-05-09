ECE590 -- Engineering Robust Server Software -- 4.26.2018


Final Project: Amazon 


Connor Grehlinger, Arda Icmez   


This project encorporates the following programming concepts:
-> Python3 & Django 

-> Client-Server communication via google protocol buffers 

-> C++ Multi-Threading 

-> Docker

-> PostgreSQL


NOTE: This repository does not contain the world simulator. According
to our interoperability group protocol specification, the host running
the UPS server is responsible for running the world simulator, connecting
to the world simulator, then relaying data such as world id to the amazon
daemon. The amazon daemon then connects to the world simulator runnning
on the UPS server's host machine using the received world id.

^ Defunct, adding the world sim now for the purpose of github ref material
Will be in world/ directory 




To deploy the Amazon web server and Amazon daemon, enter command:

'sudo docker-compose build && sudo docker-compose up'

If the amazon backend server (daemon) terminates with an error that
reads "An HTTP request took too long to complete..." you can re-run
the same command above with 'COMPOSE_HTTP_TIMEOUT=300' to give the
backend server more time to receive a response from the web server.
Example:

'sudo docker-compose build && sudo COMPOSE_HTTP_TIMEOUT=300 docker-compose up'

