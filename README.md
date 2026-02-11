# oclsim – OpenCL Simulation Framework

`oclsim` is a small C/OpenCL framework for running grid-based simulations on CPUs or GPUs using OpenCL.
It provides a minimal host-side API to manage OpenCL setup, kernel execution, double-buffered state
updates, and measurements.

This repository includes two example simulations:
- a **2D Ising model**
- a **Mandelbrot set** computation

---

## Project Structure

### Core framework
- **`oclsim.c / oclsim.h`**  
  Host-side OpenCL abstraction layer.
  Handles:
  - platform and device selection
  - context and command queue creation
  - kernel compilation and loading
  - double-buffered state handling
  - measurement buffer management

This is the core of the project. New simulations only need to provide kernels and small driver programs.

---

### Ising model example

- **`ising.c`**  
  Runs a 2D Ising model for a temperature sweep.
  Prints, for each temperature:
  - average magnetization
  - RMS magnetization

- **`isingview.c`**  
  Visual version of the Ising model.
  Displays the lattice evolution directly in the terminal using ANSI colors.

- **`ising.h`**  
  Shared definitions between host and OpenCL code:
  - lattice size
  - simulation constants
  - data structures
  - OpenCL execution dimensions

- **`ising.cl`**  
  OpenCL kernels implementing:
  - lattice initialization with random spins
  - checkerboard Metropolis updates
  - magnetization and state measurement

---

### Mandelbrot example

- **`mandel.c`**  
  Computes the Mandelbrot set using OpenCL and outputs iteration and magnitude data.

- **`mandel.h`**  
  Defines parameters such as:
  - image resolution
  - iteration count
  - complex plane coordinates

- **`mandel.cl`**  
  OpenCL kernels for:
  - initializing complex coordinates
  - iterating the Mandelbrot equation
  - collecting final values

- **`mandel_plot.m`**  
  Octave script to convert Mandelbrot output data into a PNG image.

---

## Building

Requirements:
- OpenCL headers and runtime
- GCC
- (optional) Octave for Mandelbrot plotting

Build the default program (`ising`):

```sh
make
```

Build a specific program:

```sh
make PGR=ising
make PGR=isingview
make PGR=mandel
```

All binaries are placed in the `build/` directory.

---

## Running

### Ising simulation (numeric output)

```sh
./build/ising
```

Output format:
```
temperature  mean_magnetization  rms_magnetization
```

---

### Ising visualization

```sh
./build/isingview
```

This clears the terminal and animates the lattice evolution.

---

### Mandelbrot set

Using the helper script:

```sh
./run.sh mandel
```

This will:
1. run the Mandelbrot simulation
2. save raw data to `build/mandel.dat`
3. generate an image `build/mandel.png` (requires Octave)

---

## Notes

- Platform and device selection is done via indices in `cls_new_sys(platform, device)`.
- Kernel names are fixed:
  - `init_k`
  - `update_k`
  - `measure_k`
- Host and OpenCL code share struct definitions via the same header files.

---

## License

This project is licensed under the **GNU General Public License v3 (GPLv3)**.
See the source files for full license headers.

