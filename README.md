# ♟️ Chess Engine in C++ (SDL3 + Stockfish)

## 📌 Overview

A fully completed chess application built entirely in C++, featuring a custom rule engine, modern SDL3-based graphical interface, and AI gameplay using the Stockfish engine via the UCI protocol.

This project was developed from scratch, with explicit implementation of chess rules, move validation, game state management, and UI state transitions — without relying on external chess libraries.

---

## 🎮 Features

Complete chess rules implementation
Player vs Player mode
Player vs AI (Stockfish) mode
Adjustable AI difficulty (depth 1–12)
Custom SDL3 graphical interface
Smooth board rendering and piece movement
Pawn promotion UI with piece selection
Exit confirmation dialog (ESC-safe across all states)
Robust game state handling

---

## 🎯 Design Principles

Object-Oriented Programming (OOP)
Explicit state management
Deterministic behavior
No hidden global side effects
Readable, maintainable code

---

## 🧠 Chess Rules Implemented

Legal move generation for all pieces
Turn management and color enforcement
Castling (king-side and queen-side)
En passant
Pawn promotion (Queen, Rook, Bishop, Knight)
Check and checkmate detection
Illegal move prevention

---

## 🤖 AI Integration (Stockfish)

Stockfish integrated using the UCI protocol
Bidirectional communication via pipes
Board state serialized into UCI format
Best-move parsing and coordinate translation
AI always plays Black
Difficulty controlled via search depth

---

## 🖥️ User Interface

Built using SDL3 and SDL_ttf
Start menu with:
Play vs Human
Play vs Stockfish
AI depth selection
In-game board scaling with window size
Semi-transparent overlays
Rounded buttons and dialogs
Consistent input handling across all UI states

---

## 🧩 UI States

START_MENU
PLAYING
EXIT_CONFIRM

ESC opens exit confirmation from any state
State transitions are explicitly controlled
No overlapping or hidden UI layers

---

## 🏗️ Project Architecture

├── Board -> Game state, rules, move validation
├── Piece -> Piece-specific logic and rendering
├── Engine -> Stockfish (UCI) communication
├── Settings -> Game configuration (AI, depth)
├── UI / Main -> Rendering, events, state machine

---

## 🧪 Stability & Reliability

Engine I/O synchronized safely
Promotion logic protected from turn corruption
Engine only moves on its turn
UI and logic fully decoupled
Safe cleanup of SDL and engine resources

---

## 🚀 Future Enhancements (Optional)

Move history panel
Game save/load support
Animations and sound effects
Cross-platform builds
Online multiplayer

---

## ✍️ Author

Developed by Divyansh Sharma

---

## ⭐ Final Note

This project demonstrates:
Deep understanding of game logic
Strong C++ and OOP fundamentals
Real-world engine integration
UI state management
System-level debugging skills
