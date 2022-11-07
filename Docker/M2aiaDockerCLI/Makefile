THIS-DIR=/home/maia/Documents/maia/00_my_m2aia/Docker/M2aiaDockerCLI# without trailing / ; do not remove the # character after the path (dont introduce whitespace in the path)

all:
	docker build . -t m2aia-cli
	docker run -d -it --name=m2aia-container --mount "type=bind,source=$(THIS-DIR)/m2aia-share,destination=/m2aia-share" m2aia-cli
	@echo "Build successful! Run 'docker exec -it m2aia-container bash' to get an interactive prompt."

