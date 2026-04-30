# Mini Compiler Web Application

A full-stack web application that serves as a mini compiler front-end. The project implements a C-based compiler core (lexer, parser, symbol table) for processing code snippets, a Node.js backend to execute this core, and a clean web-based frontend for code input and visualization of the compiler's output.

## Features
- **C-based Compiler Core**: Custom implementation of Lexical Analysis (Tokens), Syntax Analysis (Parsing/AST), and Semantic Symbol Table generation.
- **Node.js Backend**: An Express server that takes incoming code snippets, invokes the underlying C compiler executable, and returns the structured results.
- **Interactive Web Frontend**: Built with pure HTML, CSS, and JavaScript to provide a user-friendly UI for writing code, viewing tokens, syntactic structures, and the symbol table.

## Project Structure

```
AT_CP/
├── backend/
│   ├── package.json
│   └── server.js        # Express Node.js server
├── compiler/
│   ├── lexer.c & .h     # Lexical Analyzer
│   ├── parser.c & .h    # Syntax Analyzer
│   ├── symtab.c & .h    # Symbol Table
│   ├── main.c           # Compiler Entry Point
│   └── compiler.exe     # Compiled C executable
└── frontend/
    ├── index.html       # Web UI
    ├── style.css        # UI Styling (Glassmorphism/Dark Mode aesthetics)
    └── app.js           # Frontend Logic and API Interaction
```

## Setup & Running the Application

### 1. Backend Server
You must run the Node.js backend to let the frontend execute the compiler.

1. Open a terminal and navigate to the `backend/` directory:
   ```bash
   cd backend
   ```
2. Install the necessary Node.js dependencies:
   ```bash
   npm install
   ```
3. Start the Express server:
   ```bash
   node server.js
   ```
   *The server will typically start on `http://localhost:3000`.*

### 2. Frontend Interface
You can run the frontend by simply opening the `frontend/index.html` file in your preferred web browser. 

Alternatively, if you are using VS Code, you can use an extension like **Live Server** to serve the frontend folder.

## Usage
1. Make sure your local Node.js backend is running.
2. Open the frontend and type some code into the editor.
3. Click the **Compile** button.
4. The output panel will display the generated Tokens, parsing status, and the variables recorded in the Symbol Table.

## Authors & License
This project was built for educational purposes to demonstrate compiler frontend mechanics on the web.
