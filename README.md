# accel
Kinet GPU-accelerated cryptography library (NTT, TFHE, BLS, ML-KEM, MSM)

# Building

## kinet-gpu

```bash
cmake -B deps/kinet-gpu/build -S deps/kinet-gpu -G Ninja -DCMAKE_BUILD_TYPE=Release

cmake --build deps/kinet-gpu/build --config Release


```

## accel

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

run: cmake --build build --config Release

```