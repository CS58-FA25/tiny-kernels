# Docker
If you haven't installed the Dartmouth VPN and are developing on a mac...

```
docker run -v $(pwd):/app -it --platform=linux/amd64 amd64/ubuntu:latest /bin/bash
```

cd /app && ./.docker/build.sh
