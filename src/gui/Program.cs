using System;
using System.IO;
using System.Windows;
using System.Windows.Threading;

namespace DB.Workbench;

internal static class Program
{
    [STAThread]
    public static void Main()
    {
        string logPath = Path.Combine(AppContext.BaseDirectory, "gui-start.log");
        try
        {
            File.AppendAllText(logPath, $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} starting GUI\n");

            App app = new();
            app.InitializeComponent();
            app.ShutdownMode = ShutdownMode.OnMainWindowClose;
            app.DispatcherUnhandledException += (_, e) =>
            {
                File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "gui-error.log"), e.Exception.ToString());
                MessageBox.Show(e.Exception.ToString(), "DB GUI runtime error");
                e.Handled = true;
            };

            QueryWorkbench window = new();
            app.Run(window);

            File.AppendAllText(logPath, $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} GUI closed normally\n");
        }
        catch (Exception ex)
        {
            string errorPath = Path.Combine(AppContext.BaseDirectory, "gui-error.log");
            File.WriteAllText(errorPath, ex.ToString());
            MessageBox.Show(ex.ToString(), "DB GUI startup error");
        }
    }
}
