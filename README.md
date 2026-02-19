# FlowLang 🚀
> *AI-Powered Programming Language - Write Code Smarter, Not Harder*

## The Vision

FlowLang is a modern programming language built from the ground up to **integrate AI assistance seamlessly** into the development workflow. Instead of writing code and then asking AI for help, FlowLang lets you code *with* AI as a first-class citizen.

### Why FlowLang?

Traditional programming requires context-switching between your IDE and AI tools. FlowLang eliminates that friction:

- ✨ **Native AI Integration** - AI isn't bolted on; it's built into the language
- 🎯 **Intuitive Syntax** - Clean, readable code that feels natural to write
- ⚡ **Fast Execution** - Compiled C++ backend for production performance
- 🔄 **Human + AI Collaboration** - Write the logic, let AI handle boilerplate
- 📚 **Self-Documenting** - Code clarity that both humans and AI understand

## Quick Example

```flowlang
# Calculate fibonacci with AI suggestions
fn fibonacci(n: int) -> int {
  if n <= 1 {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

# AI can help optimize: suggest using memoization
@ai-optimize
fn fibonacci_fast(n: int) -> int {
  // AI suggests memoization pattern
}

# Print result
print(fibonacci(10));  # Output: 55
```

## Current Features

- 🏗️ **Language Foundation** - Lexer, Parser, and Interpreter
- 💡 **AI Framework** - Ready for integration with OpenAI/Local models
- 🎨 **Clean Syntax** - Python-like with type hints
- 📦 **C++ Backend** - High performance compilation

## Getting Started

### Prerequisites
- C++17 or higher
- CMake 3.16+
- ImGui (included as submodule)

### Installation

```bash
git clone https://github.com/Owenjz20/FlowLang.git
cd FlowLang
mkdir build
cd build
cmake ..
make
```

### Your First Program

Create `hello.flow`:
```flowlang
print("Hello from FlowLang!");
```

Run it:
```bash
./flowlang hello.flow
```

## Roadmap

### Phase 1: MVP (Q1 2026)
- [x] Project Setup
- [ ] Core Language Features
  - Variables and basic types
  - Control flow (if/for/while)
  - Functions
- [ ] Basic AI Integration Framework
- [ ] Documentation & Examples

### Phase 2: AI Integration (Q2 2026)
- [ ] OpenAI API Integration
- [ ] Code completion suggestions
- [ ] Bug detection and fixes
- [ ] Optimization hints

### Phase 3: Community (Q3 2026)
- [ ] Package Manager
- [ ] Standard Library
- [ ] Community Contributions
- [ ] IDE Plugins (VS Code)

## How It Works

### Traditional Workflow
```
Write Code → Run Code → Ask AI → Modify Code → Repeat
```

### FlowLang Workflow
```
Write Code with AI Suggestions → Run Code → Auto-Optimize
```

The AI is integrated into the compilation process, providing real-time assistance.

## Architecture

```
FlowLang Code
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Generation)
    ↓
AI Analyzer (Optional optimization/suggestions)
    ↓
Compiler (to bytecode/C++)
    ↓
Runtime (Execution)
```

## Contributing

We're looking for contributors! Whether you're interested in:
- **Language Design** - Help shape the syntax and features
- **AI Integration** - Work on the AI framework
- **Performance** - Optimize the compiler and runtime
- **Documentation** - Help users get started

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Project Structure

```
FlowLang/
├── src/
│   ├── lexer/          # Tokenization
│   ├── parser/         # AST generation
│   ├── compiler/       # Compilation logic
│   ├── runtime/        # Execution engine
│   └── ai/             # AI integration
├── examples/           # Example programs
├── tests/              # Unit & integration tests
├── docs/               # Documentation
└── Flowlang/           # Main application with UI
```

## FAQ

**Q: Is FlowLang production-ready?**  
A: Not yet! We're actively developing the MVP. Follow the project for updates.

**Q: Will FlowLang be open-source?**  
A: Yes! Licensed under GPL-3.0.

**Q: What AI models does FlowLang support?**  
A: Currently targeting OpenAI GPT-4, with plans for local models and other APIs.

**Q: Can I use FlowLang for my project?**  
A: Once the MVP is complete, absolutely! We'll have clear documentation.

## Support

- 📖 [Documentation](https://owenjz20.github.io/)
- 💬 [Discussions](https://github.com/Owenjz20/FlowLang/discussions)
- 🐛 [Report Issues](https://github.com/Owenjz20/FlowLang/issues)

## License

FlowLang is licensed under the [GNU General Public License v3.0](LICENSE)

## Acknowledgments

Built with passion using:
- **C++** - Core language implementation
- **ImGui** - User interface framework
- **AI** - Making programming smarter

---

**Join us in building the future of programming!** ⭐ Star this repo if you find it interesting!