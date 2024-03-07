# Python3 Host-Info

It is a script that is used to view the webhosting information, you can see the header, ping, DNS Domain, response time

## Usage
	host-info.sh
	-f/--file Host file.
	-p/--ping Test response time [valid: log, term]
	-d/--dns Information about the domain system [valid: log, term]

	Create path.txt file:
		www.google.com
		www.youtube.com

## Intallation
	git clone https://github.com/frerd7/Project.git
	cd ./Project/swh
	./host-info.sh --file path.txt --dns log --ping term
