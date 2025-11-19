# Checkpoint 5
- We did make the checkpoint! Only issue is with the x11 graphical interfaces. They don't pop up at all? main issue is with this test `./yalnix ./user/cp5_tests/test2 read -lk 0`
^ actually figured out the issue with this. I just didn't use the `-x` option for X support lol.

# Team
- Isabella Fusari
- Ahmed Al Sunbati

## Docker (if not on thayer machine)
### Build
docker buildx build --platform linux/amd64 .docker -t yalnix

### Run
./.docker/run.sh

checking