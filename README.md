# juce-limiter-plugin

This project is a real-time limiter implemented in C++ using the JUCE framework. It includes threshold and output gain controls, along with a gain-reduction meter that displays instantaneous gain reduction and a peak-hold indicator.

## Features

- Threshold and output gain parameters
- Real-time gain reduction meter with smoothing and peak-hold
- Per-sample limiter envelope with instantaneous attack and exponential release
- Stereo audio processing
- Custom-drawn graphical interface

## Technical Overview

The limiter uses an envelope-based gain computer. When a signal exceeds the threshold, the gain is reduced immediately. Recovery uses an exponential release coefficient derived from the sample rate. Maximum gain reduction per block is stored in an atomic variable and read by the GUI thread at a fixed update rate.

The editor applies its own smoothing for visual presentation and renders a vertical bar to represent the reduction amount, along with a peak-hold marker that decays over time.

## Project Structure

Source/
  PluginProcessor.h/.cpp    DSP processing, parameters, and state
  PluginEditor.h/.cpp       GUI layout and gain reduction meter
  limiter-plugin.jucer      JUCE project configuration

## Build Instructions

Open the limiter-plugin.jucer file using the JUCE Projucer and export the project to Visual Studio, Xcode, or another supported IDE. Build the VST3 target. The compiled plugin will be created under:

Builds/VisualStudio2022/x64/Debug/VST3/limiter-plugin.vst3

## Purpose

This project was developed as a portfolio piece to demonstrate real-time audio processing, JUCE-based plugin development, and practical understanding of limiter behavior and UI metering.
