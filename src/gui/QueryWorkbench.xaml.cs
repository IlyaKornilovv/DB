using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace DB.Workbench;

public partial class QueryWorkbench : Window
{
    private bool _connected;

    private static readonly Dictionary<string, string> Examples = new()
    {
        ["Employees demo"] = "CREATE DATABASE company;\nUSE company;\nCREATE TABLE employees (id INT AUTO_INCREMENT, name TEXT, email TEXT UNIQUE, salary FLOAT);\nINSERT INTO employees (name, email, salary) VALUES ('Alice', 'alice@mail.com', 50000);\nINSERT INTO employees (name, email, salary) VALUES ('Bob', 'bob@mail.com', 60000);\nSELECT * FROM employees;",
        ["Products with TEXT[]"] = "CREATE DATABASE shop;\nUSE shop;\nCREATE TABLE products (id INT, name TEXT, tags TEXT[]);\nINSERT INTO products VALUES (1, 'Laptop', '[\"electronics\",\"computers\"]');\nINSERT INTO products VALUES (2, 'Book', '[\"education\",\"books\"]');\nSELECT * FROM products;",
        ["Show catalog"] = "SHOW DATABASES;\nSHOW TABLES;",
        ["Clean demo database"] = "DROP DATABASE company;\nDROP DATABASE shop;\nSHOW DATABASES;"
    };

    public QueryWorkbench()
    {
        InitializeComponent();
        if (ExampleBox != null)
            ExampleBox.SelectedIndex = 0;
        LoadExample("Employees demo");
        KeyDown += (_, e) =>
        {
            if ((Keyboard.Modifiers == ModifierKeys.Control && e.Key == Key.Enter) || e.Key == Key.F5)
            {
                Execute_Click(this, new RoutedEventArgs());
                e.Handled = true;
            }
            else if (e.Key == Key.F6)
            {
                Clear_Click(this, new RoutedEventArgs());
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                Disconnect_Click(this, new RoutedEventArgs());
                e.Handled = true;
            }
        };
    }

    private async void Connect_Click(object sender, RoutedEventArgs e)
    {
        bool ok = await ProbeServerAsync();
        _connected = ok;
        ConnectionStateText.Text = ok ? "Connected" : "Disconnected";
        ConnectionStateText.Foreground = ok ? System.Windows.Media.Brushes.LightGreen : System.Windows.Media.Brushes.OrangeRed;
        StatusText.Text = ok ? $"Connected to {HostBox.Text}:{PortBox.Text}" : "Cannot connect. Check that the C++ server is running.";
    }

    private void Disconnect_Click(object sender, RoutedEventArgs e)
    {
        _connected = false;
        ConnectionStateText.Text = "Disconnected";
        ConnectionStateText.Foreground = System.Windows.Media.Brushes.LightGray;
        StatusText.Text = "Disconnected";
    }

    private async void ConnectionTest_Click(object sender, RoutedEventArgs e)
    {
        bool ok = await ProbeServerAsync();
        StatusText.Text = ok ? "Connection test passed" : "Connection test failed";
    }

    private async void Execute_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!_connected)
            {
                bool ok = await ProbeServerAsync();
                if (!ok)
                {
                    StatusText.Text = "Server is not available. Start: ./build/customdb_server --host 0.0.0.0 --port 5432";
                    return;
                }
                _connected = true;
                ConnectionStateText.Text = "Connected";
                ConnectionStateText.Foreground = System.Windows.Media.Brushes.LightGreen;
            }

            StatusText.Text = "Executing SQL...";
            string json = await SendSqlAsync(SqlBox.Text);
            ResponseBox.Text = PrettyJson(json);
            FillTable(json);
            StatusText.Text = "Query completed";
        }
        catch (Exception ex)
        {
            ResultGrid.ItemsSource = null;
            ResponseBox.Text = ex.ToString();
            StatusText.Text = "Query failed: " + ex.Message;
        }
    }

    private void Clear_Click(object sender, RoutedEventArgs e)
    {
        SqlBox.Clear();
        ResponseBox.Clear();
        ResultGrid.ItemsSource = null;
        StatusText.Text = "Cleared";
    }

    private void ExampleBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (SqlBox == null)
            return;

        if (ExampleBox.SelectedItem is ComboBoxItem item && item.Content is string name)
            LoadExample(name);
    }

    private void LoadExample(string name)
    {
        if (SqlBox == null)
            return;

        if (Examples.TryGetValue(name, out var sql))
            SqlBox.Text = sql;
    }

    private async Task<bool> ProbeServerAsync()
    {
        try
        {
            using TcpClient client = new();
            Task connectTask = client.ConnectAsync(HostBox.Text.Trim(), ReadPort());
            Task timeoutTask = Task.Delay(1800);
            Task winner = await Task.WhenAny(connectTask, timeoutTask);
            return winner == connectTask && client.Connected;
        }
        catch
        {
            return false;
        }
    }

    private async Task<string> SendSqlAsync(string sql)
    {
        using TcpClient client = new();
        await client.ConnectAsync(HostBox.Text.Trim(), ReadPort());
        await using NetworkStream stream = client.GetStream();

        byte[] payload = Encoding.UTF8.GetBytes(sql ?? string.Empty);
        await WriteUInt32Async(stream, (uint)payload.Length);
        await stream.WriteAsync(payload, 0, payload.Length);

        _ = await ReadUInt32Async(stream); // status: 0 = ok, 1 = error; JSON contains detailed result.
        uint length = await ReadUInt32Async(stream);
        byte[] body = await ReadExactlyAsync(stream, checked((int)length));
        return Encoding.UTF8.GetString(body);
    }

    private int ReadPort() => int.TryParse(PortBox.Text.Trim(), out int port) ? port : 5432;

    private static async Task WriteUInt32Async(Stream stream, uint value)
    {
        byte[] bytes =
        {
            (byte)(value >> 24),
            (byte)(value >> 16),
            (byte)(value >> 8),
            (byte)value
        };
        await stream.WriteAsync(bytes, 0, bytes.Length);
    }

    private static async Task<uint> ReadUInt32Async(Stream stream)
    {
        byte[] bytes = await ReadExactlyAsync(stream, 4);
        return ((uint)bytes[0] << 24) | ((uint)bytes[1] << 16) | ((uint)bytes[2] << 8) | bytes[3];
    }

    private static async Task<byte[]> ReadExactlyAsync(Stream stream, int length)
    {
        byte[] buffer = new byte[length];
        int offset = 0;
        while (offset < length)
        {
            int read = await stream.ReadAsync(buffer, offset, length - offset);
            if (read == 0) throw new EndOfStreamException("Connection closed before full response was received.");
            offset += read;
        }
        return buffer;
    }

    private static string PrettyJson(string json)
    {
        try
        {
            using JsonDocument doc = JsonDocument.Parse(json);
            return JsonSerializer.Serialize(doc.RootElement, new JsonSerializerOptions { WriteIndented = true });
        }
        catch
        {
            return json;
        }
    }

    private void FillTable(string json)
    {
        ResultGrid.ItemsSource = null;
        using JsonDocument doc = JsonDocument.Parse(json);
        JsonElement root = doc.RootElement;
        if (!root.TryGetProperty("columns", out JsonElement columns) || !root.TryGetProperty("rows", out JsonElement rows))
            return;

        DataTable table = new();
        foreach (JsonElement column in columns.EnumerateArray())
            table.Columns.Add(column.GetString() ?? "column");

        foreach (JsonElement row in rows.EnumerateArray())
        {
            DataRow dataRow = table.NewRow();
            int index = 0;
            foreach (JsonElement value in row.EnumerateArray())
            {
                if (index < table.Columns.Count)
                    dataRow[index] = value.ValueKind == JsonValueKind.String ? value.GetString() ?? string.Empty : value.ToString();
                index++;
            }
            table.Rows.Add(dataRow);
        }
        ResultGrid.ItemsSource = table.DefaultView;
    }
}
