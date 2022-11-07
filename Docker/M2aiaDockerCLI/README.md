# M2aia-CLI
This is used as the Command Line Interface for M2aia. 

## Prerequisites
- Working installation of `docker`

- User that is part of the `docker` group

## Configuration
Currently, to use the CLI, you will have to configure the `Makefile` in this directory. 
For this, open the `Makefile` using your favourite editor. 
Then modify the variable `THIS-DIR` to point to the current directory (the directory where this `README.md` and the `Makefile` are located), since docker does not allow relative paths. 

That's it! 

You can now use all programs inside that docker container (including the ones in `$(THIS-DIR)/bin`). 

## Run the CLI
There are two ways supported: 

 - From within M2aia
 - Headless, without M2aia itself

### Within M2aia
TODO ^^'

### Headless
For running in headless mode, please just run the `make` command in this directory. 
If everything has been configured correctly, a docker container named `m2aia-container` should start up. 
You can get a command prompt inside the container by executing `docker exec -it m2aia-container bash`. 
Inside the `m2aia-container` you can find a directory named `/m2aia-share` which is mapped to the `$(THIS-DIR)/m2aia-share` directory. 
The `$(THIS-DIR)/m2aia-share` directory is used for storing the outputs of CLI-Modules on the Host. 
If done correctly, M2aia should be able to read the files in that directory. 

