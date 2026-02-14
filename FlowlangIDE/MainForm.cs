using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace FlowLangIDE
{
    public class MainForm : Form
    {
        private TextBox editor;
        private TextBox console;

        private Button openLangButton;
        private Button saveLangButton;
        private Button openFlowButton;
        private Button saveFlowButton;
        private Button generateButton;
        private Button runButton;

        private string currentLangFile = "";
        private string currentFlowFile = "";

        public MainForm()
        {
            Text = "FlowLang IDE";
            Width = 1100;
            Height = 750;

            editor = new TextBox
            {
                Multiline = true,
                ScrollBars = ScrollBars.Both,
                Font = new System.Drawing.Font("Consolas", 11),
                Dock = DockStyle.Top,
                Height = 450,
                AcceptsTab = true
            };

            console = new TextBox
            {
                Multiline = true,
                ScrollBars = ScrollBars.Vertical,
                Font = new System.Drawing.Font("Consolas", 10),
                Dock = DockStyle.Fill,
                ReadOnly = true
            };

            openLangButton = MakeButton("Open .lang", OpenLang);
            saveLangButton = MakeButton("Save .lang", SaveLang);
            openFlowButton = MakeButton("Open .flow", OpenFlow);
            saveFlowButton = MakeButton("Save .flow", SaveFlow);
            generateButton = MakeButton("Generate Flow", GenerateFlow);
            runButton = MakeButton("Run Flow", RunFlow);

            var top = new Panel { Dock = DockStyle.Top, Height = 32 };
            top.Controls.Add(runButton);
            top.Controls.Add(generateButton);
            top.Controls.Add(saveFlowButton);
            top.Controls.Add(openFlowButton);
            top.Controls.Add(saveLangButton);
            top.Controls.Add(openLangButton);

            Controls.Add(console);
            Controls.Add(editor);
            Controls.Add(top);

            editor.Text = "Describe your program here in English (.lang)...";
        }

        private Button MakeButton(string text, EventHandler onClick)
        {
            var b = new Button
            {
                Text = text,
                AutoSize = true,
                Dock = DockStyle.Left
            };
            b.Click += onClick;
            return b;
        }

        private void OpenLang(object? sender, EventArgs e)
        {
            using var dlg = new OpenFileDialog
            {
                Filter = "FlowLang AI files (*.lang)|*.lang|All files (*.*)|*.*"
            };
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                currentLangFile = dlg.FileName;
                editor.Text = File.ReadAllText(currentLangFile);
                Text = "FlowLang IDE - " + Path.GetFileName(currentLangFile);
            }
        }

        private void SaveLang(object? sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(currentLangFile))
            {
                using var dlg = new SaveFileDialog
                {
                    Filter = "FlowLang AI files (*.lang)|*.lang|All files (*.*)|*.*",
                    FileName = "program.lang"
                };
                if (dlg.ShowDialog() != DialogResult.OK)
                    return;
                currentLangFile = dlg.FileName;
            }

            File.WriteAllText(currentLangFile, editor.Text, Encoding.UTF8);
            Text = "FlowLang IDE - " + Path.GetFileName(currentLangFile);
        }

        private void OpenFlow(object? sender, EventArgs e)
        {
            using var dlg = new OpenFileDialog
            {
                Filter = "FlowLang code files (*.flow)|*.flow|All files (*.*)|*.*"
            };
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                currentFlowFile = dlg.FileName;
                editor.Text = File.ReadAllText(currentFlowFile);
                Text = "FlowLang IDE - " + Path.GetFileName(currentFlowFile);
            }
        }

        private void SaveFlow(object? sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(currentFlowFile))
            {
                using var dlg = new SaveFileDialog
                {
                    Filter = "FlowLang code files (*.flow)|*.flow|All files (*.*)|*.*",
                    FileName = "program.flow"
                };
                if (dlg.ShowDialog() != DialogResult.OK)
                    return;
                currentFlowFile = dlg.FileName;
            }

            File.WriteAllText(currentFlowFile, editor.Text, Encoding.UTF8);
            Text = "FlowLang IDE - " + Path.GetFileName(currentFlowFile);
        }

        private void GenerateFlow(object? sender, EventArgs e)
        {
            console.Clear();

            if (string.IsNullOrEmpty(currentLangFile))
            {
                using var dlg = new SaveFileDialog
                {
                    Filter = "FlowLang AI files (*.lang)|*.lang",
                    FileName = "program.lang"
                };
                if (dlg.ShowDialog() != DialogResult.OK)
                    return;
                currentLangFile = dlg.FileName;
            }

            File.WriteAllText(currentLangFile, editor.Text, Encoding.UTF8);

            var psi = new ProcessStartInfo
            {
                FileName = "FlowLangAI.exe",
                Arguments = $"\"{currentLangFile}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            try
            {
                var proc = Process.Start(psi);
                string stdout = proc!.StandardOutput.ReadToEnd();
                string stderr = proc.StandardError.ReadToEnd();
                proc.WaitForExit();

                console.AppendText(stdout);
                if (!string.IsNullOrWhiteSpace(stderr))
                    console.AppendText(Environment.NewLine + stderr);

                string generatedFlow = Path.ChangeExtension(currentLangFile, ".flow");
                if (File.Exists(generatedFlow))
                {
                    currentFlowFile = generatedFlow;
                    editor.Text = File.ReadAllText(generatedFlow);
                    Text = "FlowLang IDE - " + Path.GetFileName(currentFlowFile);
                }
                else
                {
                    console.AppendText(Environment.NewLine + "No .flow file generated.");
                }
            }
            catch (Exception ex)
            {
                console.AppendText("Error running FlowLangAI.exe: " + ex.Message);
            }
        }

        private void RunFlow(object? sender, EventArgs e)
        {
            console.Clear();

            if (string.IsNullOrEmpty(currentFlowFile))
            {
                using var dlg = new SaveFileDialog
                {
                    Filter = "FlowLang code files (*.flow)|*.flow",
                    FileName = "program.flow"
                };
                if (dlg.ShowDialog() != DialogResult.OK)
                    return;
                currentFlowFile = dlg.FileName;
            }

            File.WriteAllText(currentFlowFile, editor.Text, Encoding.UTF8);

            var psi = new ProcessStartInfo
            {
                FileName = "FlowLang.exe",
                Arguments = $"\"{currentFlowFile}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            try
            {
                var proc = Process.Start(psi);
                string stdout = proc!.StandardOutput.ReadToEnd();
                string stderr = proc.StandardError.ReadToEnd();
                proc.WaitForExit();

                console.AppendText(stdout);
                if (!string.IsNullOrWhiteSpace(stderr))
                    console.AppendText(Environment.NewLine + stderr);
            }
            catch (Exception ex)
            {
                console.AppendText("Error running FlowLang.exe: " + ex.Message);
            }
        }
    }
}
