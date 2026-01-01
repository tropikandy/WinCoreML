# CoreMLWin Docker Container
# Windows Server Core base image required for DirectML support

FROM mcr.microsoft.com/windows/servercore:ltsc2022

# Metadata
LABEL maintainer="CoreMLWin Project"
LABEL description="Universal ML Runtime for Windows with DirectML GPU acceleration"
LABEL version="0.1.0"

# Set working directory
WORKDIR /coremlwin

# Copy build artifacts
COPY build/bin/Release/ ./bin/
COPY runtime/config/ ./config/
COPY sdk/python/ ./sdk/python/
COPY tools/ ./tools/

# Install Python (if not in base image)
RUN powershell -Command \
    Invoke-WebRequest -Uri https://www.python.org/ftp/python/3.11.0/python-3.11.0-amd64.exe -OutFile python-installer.exe; \
    Start-Process python-installer.exe -Wait -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1'; \
    Remove-Item python-installer.exe

# Install Python SDK
RUN pip install --no-cache-dir -e /coremlwin/sdk/python

# Expose service port (if using TCP instead of named pipes)
EXPOSE 9001

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD powershell -Command "python -c 'from coremlwin_client import CoreMLWinClient; c = CoreMLWinClient(); c.health()'"

# Run service
ENTRYPOINT ["C:\\coremlwin\\bin\\coremlwin_service.exe"]
