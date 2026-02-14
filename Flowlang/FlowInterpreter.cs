using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace FlowLangRuntime
{
    public class FlowInterpreter
    {
        private readonly Action<string> _write;

        private class FunctionDef
        {
            public List<string> Parameters { get; set; } = new();
            public List<string> BodyLines { get; set; } = new();
            public int IndentLevel { get; set; }
        }

        private readonly Dictionary<string, object?> _globals = new();
        private readonly Dictionary<string, FunctionDef> _functions = new();

        public FlowInterpreter(Action<string> write)
        {
            _write = write;
            LoadStdLib();
            LoadAddons();
        }

        private void LoadStdLib()
        {
            foreach (var kv in FlowStdLib.Functions)
                _globals[kv.Key] = kv.Value;
        }

        private void LoadAddons()
        {
            string addonsDir = Path.Combine(AppContext.BaseDirectory, "addons");
            if (!Directory.Exists(addonsDir)) return;

            foreach (var file in Directory.GetFiles(addonsDir, "*.flowaddon"))
            {
                string code = File.ReadAllText(file);
                LoadAddonCode(code);
            }
        }

        private void LoadAddonCode(string code)
        {
            var lines = NormalizeLines(code);

            for (int i = 0; i < lines.Count; i++)
            {
                string trimmed = lines[i].TrimStart();

                if (trimmed.StartsWith("func "))
                {
                    ParseFunction(lines, ref i);
                }
            }
        }

        public void RegisterFunction(string name, Func<List<object?>, object?> fn)
        {
            _globals[name] = fn;
        }

        public void Run(string source)
        {
            var lines = NormalizeLines(source);

            foreach (var line in lines)
            {
                var trimmed = line.TrimStart();
                if (trimmed.StartsWith("app "))
                {
                    var app = new FlowAppEngine(_write);
                    app.RunApp(lines);
                    return;
                }
                if (trimmed.StartsWith("game "))
                {
                    var game = new FlowGameEngine(_write);
                    game.RunGame(lines);
                    return;
                }
            }

            ExecuteScript(lines);
        }

        private List<string> NormalizeLines(string source)
        {
            var raw = source.Replace("\r\n", "\n").Split('\n');
            var list = new List<string>();
            foreach (var line in raw)
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                list.Add(line.TrimEnd());
            }
            return list;
        }

        private void ExecuteScript(List<string> lines)
        {
            var scriptBody = new List<string>();
            bool inScript = false;
            int scriptIndent = 0;

            for (int i = 0; i < lines.Count; i++)
            {
                string line = lines[i];
                int indent = CountIndent(line);
                string trimmed = line.TrimStart();

                if (!inScript)
                {
                    if (trimmed.StartsWith("script:"))
                    {
                        inScript = true;
                        scriptIndent = indent;
                    }
                    else if (trimmed.StartsWith("func "))
                    {
                        ParseFunction(lines, ref i);
                    }
                    continue;
                }
                else
                {
                    if (indent <= scriptIndent)
                        break;
                    scriptBody.Add(line);
                }
            }

            ExecuteBlock(scriptBody, 0, scriptBody.Count, _globals);
        }

        private void ParseFunction(List<string> lines, ref int index)
        {
            string header = lines[index];
            int indent = CountIndent(header);
            string trimmed = header.TrimStart();

            int nameStart = "func ".Length;
            int parenIndex = trimmed.IndexOf('(', nameStart);
            int parenClose = trimmed.IndexOf(')', parenIndex);

            string name = trimmed.Substring(nameStart, parenIndex - nameStart).Trim();
            string paramList = trimmed.Substring(parenIndex + 1, parenClose - parenIndex - 1).Trim();

            var func = new FunctionDef { IndentLevel = indent };

            if (!string.IsNullOrWhiteSpace(paramList))
            {
                foreach (var p in paramList.Split(',', StringSplitOptions.RemoveEmptyEntries))
                    func.Parameters.Add(p.Trim());
            }

            index++;
            while (index < lines.Count)
            {
                string line = lines[index];
                int lineIndent = CountIndent(line);
                if (string.IsNullOrWhiteSpace(line))
                {
                    index++;
                    continue;
                }
                if (lineIndent <= indent)
                {
                    index--;
                    break;
                }
                func.BodyLines.Add(line);
                index++;
            }

            _functions[name] = func;
        }

        private int CountIndent(string line)
        {
            int count = 0;
            foreach (char c in line)
            {
                if (c == ' ') count++;
                else break;
            }
            return count;
        }

        private object? ExecuteBlock(List<string> lines, int start, int end, Dictionary<string, object?> scope)
        {
            int i = start;
            while (i < end)
            {
                string line = lines[i];
                string trimmed = line.TrimStart();

                if (trimmed.StartsWith("if "))
                {
                    i = ExecuteIf(lines, i, scope);
                }
                else if (trimmed.StartsWith("while "))
                {
                    i = ExecuteWhile(lines, i, scope);
                }
                else if (trimmed.StartsWith("return "))
                {
                    var expr = trimmed.Substring("return ".Length);
                    return EvalExpression(expr, scope);
                }
                else if (trimmed.StartsWith("print "))
                {
                    var expr = trimmed.Substring("print ".Length);
                    var val = EvalExpression(expr, scope);
                    _write(ValueToString(val));
                    i++;
                }
                else if (trimmed.StartsWith("set "))
                {
                    var rest = trimmed.Substring("set ".Length);
                    int eq = rest.IndexOf('=');
                    if (eq > 0)
                    {
                        string name = rest.Substring(0, eq).Trim();
                        string expr = rest.Substring(eq + 1).Trim();
                        scope[name] = EvalExpression(expr, scope);
                    }
                    i++;
                }
                else if (IsFunctionCall(trimmed))
                {
                    EvalExpression(trimmed, scope);
                    i++;
                }
                else
                {
                    i++;
                }
            }

            return null;
        }

        private int ExecuteIf(List<string> lines, int index, Dictionary<string, object?> scope)
        {
            string header = lines[index];
            int indent = CountIndent(header);
            string trimmed = header.TrimStart();

            int colon = trimmed.LastIndexOf(':');
            string condExpr = trimmed.Substring(3, colon - 3).Trim();

            bool cond = IsTruthy(EvalExpression(condExpr, scope));

            var ifBlock = new List<string>();
            int i = index + 1;
            while (i < lines.Count)
            {
                string line = lines[i];
                int lineIndent = CountIndent(line);
                if (string.IsNullOrWhiteSpace(line))
                {
                    i++;
                    continue;
                }
                if (lineIndent <= indent)
                    break;
                ifBlock.Add(line);
                i++;
            }

            List<string>? elseBlock = null;
            if (i < lines.Count)
            {
                string maybeElse = lines[i];
                int elseIndent = CountIndent(maybeElse);
                string trimmedElse = maybeElse.TrimStart();
                if (elseIndent == indent && trimmedElse.StartsWith("else:"))
                {
                    elseBlock = new List<string>();
                    i++;
                    while (i < lines.Count)
                    {
                        string line = lines[i];
                        int lineIndent = CountIndent(line);
                        if (string.IsNullOrWhiteSpace(line))
                        {
                            i++;
                            continue;
                        }
                        if (lineIndent <= indent)
                            break;
                        elseBlock.Add(line);
                        i++;
                    }
                }
            }

            if (cond)
            {
                ExecuteBlock(ifBlock, 0, ifBlock.Count, scope);
            }
            else if (elseBlock != null)
            {
                ExecuteBlock(elseBlock, 0, elseBlock.Count, scope);
            }

            return i;
        }

        private int ExecuteWhile(List<string> lines, int index, Dictionary<string, object?> scope)
        {
            string header = lines[index];
            int indent = CountIndent(header);
            string trimmed = header.TrimStart();

            int colon = trimmed.LastIndexOf(':');
            string condExpr = trimmed.Substring(6, colon - 6).Trim();

            var body = new List<string>();
            int i = index + 1;
            while (i < lines.Count)
            {
                string line = lines[i];
                int lineIndent = CountIndent(line);
                if (string.IsNullOrWhiteSpace(line))
                {
                    i++;
                    continue;
                }
                if (lineIndent <= indent)
                    break;
                body.Add(line);
                i++;
            }

            while (IsTruthy(EvalExpression(condExpr, scope)))
            {
                var result = ExecuteBlock(body, 0, body.Count, scope);
                if (result != null) break;
            }

            return i;
        }

        private bool IsFunctionCall(string trimmed)
        {
            int paren = trimmed.IndexOf('(');
            int end = trimmed.LastIndexOf(')');
            if (paren > 0 && end > paren)
            {
                string name = trimmed.Substring(0, paren).Trim();
                if (_functions.ContainsKey(name)) return true;
                if (_globals.TryGetValue(name, out var val) && val is Func<List<object?>, object?>) return true;
            }
            return false;
        }

        private object? EvalExpression(string expr, Dictionary<string, object?> scope)
        {
            expr = expr.Trim();

            if (expr.StartsWith("\"") && expr.EndsWith("\"") && expr.Length >= 2)
                return expr.Substring(1, expr.Length - 2);

            if (IsFunctionCall(expr))
                return EvalFunctionCall(expr, scope);

            if (double.TryParse(expr, NumberStyles.Any, CultureInfo.InvariantCulture, out double num))
                return num;

            var tokens = TokenizeExpr(expr);
            if (tokens.Count == 1)
            {
                string t = tokens[0];
                if (scope.TryGetValue(t, out var val))
                    return val;
                if (_globals.TryGetValue(t, out var gval))
                    return gval;
                return t;
            }

            object? current = EvalSingle(tokens[0], scope);
            int i = 1;
            while (i < tokens.Count - 1)
            {
                string op = tokens[i];
                object? right = EvalSingle(tokens[i + 1], scope);
                current = ApplyOp(current, op, right);
                i += 2;
            }

            return current;
        }

        private List<string> TokenizeExpr(string expr)
        {
            var list = new List<string>();
            int i = 0;
            while (i < expr.Length)
            {
                if (char.IsWhiteSpace(expr[i]))
                {
                    i++;
                    continue;
                }
                if ("()+-*/<>!=,".IndexOf(expr[i]) >= 0)
                {
                    if (i + 1 < expr.Length)
                    {
                        string two = expr.Substring(i, 2);
                        if (two == "<=" || two == ">=" || two == "==" || two == "!=")
                        {
                            list.Add(two);
                            i += 2;
                            continue;
                        }
                    }
                    list.Add(expr[i].ToString());
                    i++;
                }
                else if (expr[i] == '"')
                {
                    int start = i;
                    i++;
                    while (i < expr.Length && expr[i] != '"') i++;
                    i++;
                    list.Add(expr.Substring(start, i - start));
                }
                else
                {
                    int start = i;
                    while (i < expr.Length && !char.IsWhiteSpace(expr[i]) && "()+-*/<>!=,".IndexOf(expr[i]) < 0)
                        i++;
                    list.Add(expr.Substring(start, i - start));
                }
            }
            return list;
        }

        private object? EvalSingle(string token, Dictionary<string, object?> scope)
        {
            token = token.Trim();
            if (token.StartsWith("\"") && token.EndsWith("\""))
                return token.Substring(1, token.Length - 2);

            if (double.TryParse(token, NumberStyles.Any, CultureInfo.InvariantCulture, out double num))
                return num;

            if (scope.TryGetValue(token, out var val))
                return val;

            if (_globals.TryGetValue(token, out var gval))
                return gval;

            return token;
        }

        private object? ApplyOp(object? left, string op, object? right)
        {
            if (left is double || left is int || left is float || right is double || right is int || right is float)
            {
                double l = Convert.ToDouble(left ?? 0, CultureInfo.InvariantCulture);
                double r = Convert.ToDouble(right ?? 0, CultureInfo.InvariantCulture);

                return op switch
                {
                    "+" => l + r,
                    "-" => l - r,
                    "*" => l * r,
                    "/" => r != 0 ? l / r : 0,
                    "<" => l < r,
                    ">" => l > r,
                    "<=" => l <= r,
                    ">=" => l >= r,
                    "==" => l == r,
                    "!=" => l != r,
                    _ => null
                };
            }

            if (op == "+" && left is string ls)
                return ls + ValueToString(right);

            if (left is string ls2 && right is string rs2)
            {
                int cmp = string.Compare(ls2, rs2, StringComparison.Ordinal);
                return op switch
                {
                    "==" => cmp == 0,
                    "!=" => cmp != 0,
                    "<" => cmp < 0,
                    ">" => cmp > 0,
                    "<=" => cmp <= 0,
                    ">=" => cmp >= 0,
                    _ => null
                };
            }

            return null;
        }

        private object? EvalFunctionCall(string expr, Dictionary<string, object?> scope)
        {
            int paren = expr.IndexOf('(');
            int end = expr.LastIndexOf(')');
            string name = expr.Substring(0, paren).Trim();
            string argsPart = expr.Substring(paren + 1, end - paren - 1).Trim();

            var argValues = new List<object?>();
            if (!string.IsNullOrWhiteSpace(argsPart))
            {
                var parts = SplitArgs(argsPart);
                foreach (var p in parts)
                    argValues.Add(EvalExpression(p.Trim(), scope));
            }

            if (_functions.TryGetValue(name, out var func))
            {
                var localScope = new Dictionary<string, object?>(_globals);
                for (int i = 0; i < func.Parameters.Count; i++)
                {
                    string paramName = func.Parameters[i];
                    object? val = i < argValues.Count ? argValues[i] : null;
                    localScope[paramName] = val;
                }

                var result = ExecuteBlock(func.BodyLines, 0, func.BodyLines.Count, localScope);
                return result;
            }

            if (_globals.TryGetValue(name, out var gval) && gval is Func<List<object?>, object?> fn)
            {
                return fn(argValues);
            }

            throw new Exception("Unknown function: " + name);
        }

        private List<string> SplitArgs(string args)
        {
            var list = new List<string>();
            int depth = 0;
            int start = 0;
            for (int i = 0; i < args.Length; i++)
            {
                char c = args[i];
                if (c == '(') depth++;
                else if (c == ')') depth--;
                else if (c == ',' && depth == 0)
                {
                    list.Add(args.Substring(start, i - start));
                    start = i + 1;
                }
            }
            if (start < args.Length)
                list.Add(args.Substring(start));
            return list;
        }

        private bool IsTruthy(object? value)
        {
            if (value == null) return false;
            if (value is bool b) return b;
            if (value is double d) return d != 0;
            if (value is string s) return s.Length > 0;
            return true;
        }

        private string ValueToString(object? value)
        {
            return value switch
            {
                null => "null",
                double d => d.ToString(CultureInfo.InvariantCulture),
                _ => value.ToString() ?? ""
            };
        }
    }
}
