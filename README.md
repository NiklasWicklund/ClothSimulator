# ClothSimulator
This is a Cloth Simulator made in C++ with GLFW and GLM, made for the DH2323 Project at KTH

A short video showcasing the simulation can be found on [Youtube](https://www.youtube.com/watch?v=9gnUJxzgzdY).
# Build & Run Program

## 1. Installing dependencies (Linux)
This project requires GLFW and GLM. GLM is already included in the repo, but GLFW will have to be downloaded.

With apt-get you can install libglfw3
```
sudo apt-get install libglfw3
sudo apt-get install libglfw3-dev
``` 

## 2. Compiling the program
In the ClothSimulator directory, compile the program with

```
g++ src/main.cpp -lGL -lglfw -I "includes/glm" -o main.out
```

## 3. Run the program
Run the program with

```
./main.out
``` 
