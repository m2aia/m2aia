M²aia Online
-----------

Get the docker image:

```docker pull ghcr.io/m2aia/m2aia:latest```

Run M²aia Online:

```docker run --rm -v /local/data/path:/data -p 8888:80 ghcr.io/m2aia/m2aia:latest```

with password:

```docker run --rm -v /local/data/path:/data -e VNC_PASSWORD=<password> -p 8888:80 ghcr.io/m2aia/m2aia:latest```

Start M²aia by accessing http://localhost:8888

The container will shut down if M²aia is closed (File->Exit).
The results written to the mapped volume have root user ownership.