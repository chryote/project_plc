# ==============================================================================
# Stage 1: Build & Verification Stage
# ==============================================================================
FROM alpine:3.20 AS builder

# Install build dependencies
RUN apk add --no-cache \
    g++ \
    cmake \
    ninja \
    make \
    musl-dev \
    linux-headers

WORKDIR /src

# Copy project files
COPY CMakeLists.txt .
COPY include/ ./include/
COPY src/ ./src/
COPY tests/ ./tests/
COPY benchmarks/ ./benchmarks/

# Build project with Release optimizations
RUN cmake -B /build -S . -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DBUILD_BENCHMARKS=ON \
    && cmake --build /build

# Run automated test verification during container build
RUN /build/tests/plc_tests

# ==============================================================================
# Stage 2: Minimal, Secure Runtime Image
# ==============================================================================
FROM alpine:3.20 AS runtime

# Install minimal runtime standard libraries
RUN apk add --no-cache libstdc++

# Create non-root user for sustainable security
RUN addgroup -S appgroup && adduser -S appuser -G appgroup

# Copy compiled binaries from builder
COPY --from=builder /build/plc_tool /usr/local/bin/plc_tool
COPY --from=builder /build/tests/plc_tests /usr/local/bin/plc_tests
COPY --from=builder /build/benchmarks/plc_benchmarks /usr/local/bin/plc_benchmarks

USER appuser
WORKDIR /home/appuser

# Default configuration environment variables
ENV PLC_HOST=192.168.70.111
ENV PLC_PORT=502
ENV PLC_UNIT_ID=1

ENTRYPOINT ["plc_tool"]
CMD ["lane"]
