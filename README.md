<h1>Unix Shell Model</h1>

<h2>Description</h2>
This project is a custom-built Unix shell implemented in C, designed to provide basic command-line functionality with additional features for improved usability. The shell operates in an interactive loop, where it accepts user commands, creates child processes to execute them, and displays the output before prompting for the next input. It also supports batch mode execution, allowing users to run a sequence of commands from a file.
<br />
<h2>Key Features</h2>

<ul>
    <li>✅ <strong>Command Execution</strong> – Supports running external commands with arguments, just like a standard shell.</li>
    <li>✅ <strong>Multiple Commands</strong> – Allows users to execute multiple commands sequentially using the <code>;</code> separator.</li>
    <li>✅ <strong>Built-in Commands</strong> – Implements <code>exit</code>, <code>cd</code>, and <code>pwd</code> without requiring process creation.</li>
    <li>✅ <strong>Redirection (<code>></code> and <code>>+</code>)</strong> – Supports standard output redirection to files, including an advanced custom <code>>+</code> redirection that inserts output at the beginning of a file.</li>
    <li>✅ <strong>Whitespace Handling</strong> – Properly interprets commands regardless of spacing variations.</li>
    <li>✅ <strong>Batch Mode</strong> – Runs commands from a batch file, making it easy to automate testing and execution.</li>
</ul>

