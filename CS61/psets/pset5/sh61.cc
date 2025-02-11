#include "sh61.hh"

#include <cctype>
#include <fcntl.h>
#include <functional>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include <unordered_map>

// For the love of God
#undef exit
#define exit __DO_NOT_CALL_EXIT__READ_PROBLEM_SET_DESCRIPTION__

// Signal handler code
#define CANCELLED 0
#define NOT_CANCELLED 1

static std::unordered_map<std::string, std::string> variables; 

volatile sig_atomic_t current_status = NOT_CANCELLED;

void signal_handler(int signum) {
    (void) signum;
    current_status = CANCELLED;

    printf("\n");
    _exit(-1);
}

#define BACKGROUND_FLAG 1

// Represents runnable shell command (either single command or sequence)
//  can be run, exited, has a status, and a way to initialize it using a token iterator
struct abstract_command {
    pid_t pid = -1;

    int status;

    int operation = -1;

    int flags;

    int in_fd = 0;
    int out_fd = 1;

    virtual ~abstract_command() {}

    // Increments the iterator until reaches end of abstract command
    //  populates internal data
    virtual void parse(shell_token_iterator* it, shell_parser parser) {
        (void)it;
        (void)parser;
    }

    void command_exit() {
        if (pid == 0) {
            _exit(success() ? 0 : -1);
        }
    }

    void close_pipes() {
        // Ensure we don't close stdio
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }

        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
    }

    void setup_pipes() {
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);

        close_pipes();
    }

    virtual pid_t run() {
        return -1;
    }
    
    // Returns true if command was succesful
    bool success() {
        if (status == -1) {
            return false;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status) == 0;
        }

        if (WIFSIGNALED(status)) {
            return WTERMSIG(status) == 0;
        }

        return true;
    }

    bool is_background() {
        return operation == TYPE_BACKGROUND || (flags & BACKGROUND_FLAG) != 0;
    }

    bool is_conditional_header() {
        return operation == TYPE_OR || operation == TYPE_AND;
    }
};

// Garbage cleaning utility to prevent leaks
static std::vector<abstract_command*> garbage;

void clear_garbage() {
    for (size_t index = 0; index < garbage.size(); index++) {
        delete garbage[index];
    }

    garbage.clear();
}

// Represents a single shell command with redirections
struct command : abstract_command {
    std::vector<std::string> args;

    bool append = false;

    std::string in_file;
    std::string out_file;
    std::string err_file;

    void parse(shell_token_iterator* it, shell_parser parser) {
        std::string redirect_op;

        // Parse until we reach the end of the line or a seperator token
        while (*it != parser.end() &&
               (it->type() == 0 || it->type() == TYPE_REDIRECT_OP)) {
            if (it->type() == TYPE_REDIRECT_OP) {
                redirect_op = it->str();
            } else {
                if (!redirect_op.empty()) {
                    if (redirect_op == "<") {
                        in_file = it->str();
                    }

                    if (redirect_op == ">") {
                        out_file = it->str();
                    }

                    if (redirect_op == "2>") {
                        err_file = it->str();
                    }

                    if (redirect_op == ">>") {
                        out_file = it->str();
                        append = true;
                    }

                    redirect_op = "";
                } else {
                    // Replace variables
                    std::ostringstream replaced;
                   
                    bool in_variable = false;
                    std::ostringstream variable;

                    for (std::basic_string<char>::size_type index = 0; 
                            index < it->str().size(); ++index) {

                        char c = it->str().c_str()[index];

                        if (in_variable) {
                            // Special case if last character
                            if (index == it->str().size() - 1) {
                                variable << c;
                                c = ' ';
                            }

                            if (std::isspace((unsigned char) c)) {
                                
                                in_variable = false;
                                
                                // Perform the replacement
                                // First try environment variables
                                if (const char* env_value 
                                        = std::getenv(variable.str().c_str())) {
                                   
                                    replaced << env_value;
                                } else {
                                    std::string loc_value 
                                        = variables.at(variable.str());

                                    if (!loc_value.empty()) {
                                        replaced << loc_value;
                                    }
                                }

                                // Clear the string stream
                                variable.str("");
                                variable.clear();
                            } else  {
                                variable << c;
                            }
                        } else if (c == '$') {
                            in_variable = true; 
                        } else {
                            replaced << c;
                        }
                    }

                    args.push_back(replaced.str());
                }
            }

            ++(*it);
        }

        // Default end token is sequence
        if (*it == parser.end() &&
            it->type() != 0 && it->type() != TYPE_REDIRECT_OP) {
            operation = TYPE_SEQUENCE;
        } else {
            operation = it->type();
        }
    }

    void setup_redirection(std::string file, int redirection_fd, int settings) {
        if (!file.empty()) {
            int fd = open(file.c_str(), settings, 00777);
            if (fd < 0) {
                fprintf(stderr, "%s: No such file or directory\n", in_file.c_str());
                _exit(1);
            }
            dup2(fd, redirection_fd);
            close(fd);
        }
    }

    void setup_redirections() {
        setup_redirection(in_file, STDIN_FILENO, O_RDONLY);
        if (append) {
            setup_redirection(out_file, STDOUT_FILENO, O_WRONLY | O_CREAT | O_APPEND);
        } else {
            setup_redirection(out_file, STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC);
        }
        setup_redirection(err_file, STDERR_FILENO, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND);
    }

    pid_t run() {
        // Ensure command is non-degenerate
        if (args.size() < 1) {
            return -1;
        }

        // Check for variable setting case
        if (args.size() == 1 && (args[0].find('=') != std::string::npos)) {
            std::ostringstream variable;
            std::ostringstream value;
            
            bool in_variable = true;
            for (auto& c : args[0]) {
                if (c == '=') {
                    in_variable = false;
                } else {
                    if (in_variable) {
                        variable << c;
                    } else {
                        value << c;
                    }
                }
            }

            // Add to variable table
            variables.insert(std::pair<std::string, std::string>
                    (variable.str(), value.str()));
            
            pid = -1;
            return 0;
        }

        // Check for cd special case
        const char* cmd = args[0].c_str();

        if (strcmp(cmd, "cd") == 0) {
            status = chdir(args[1].c_str());
        }

        // Fork the process
        pid = fork();

        if (pid == 0) {
            // Use pipes for simple commands
            setup_pipes();

            // Convert args into usable form
            std::vector<const char*> argv;

            for (size_t index = 0; index < args.size(); index++) {
                argv.push_back(args[index].c_str());
            }

            // Ensure arg list is null terminated
            argv.push_back(nullptr);

            setup_redirections();

            // Try running command
            if (strcmp(cmd, "cd") != 0
                    && execvp(cmd, (char* const*)argv.data()) == -1) {
                fprintf(stderr, "%s: %s\n", cmd, strerror(errno));
                return -1;
            }

            return 0;
        }

        return pid;
    }
};

// Abstract command structure representing commands which contain lists 
//  of smaller commands
struct command_list : abstract_command {
    std::vector<abstract_command*> commands;

    void parse(shell_token_iterator* it, shell_parser parser) {
        bool should_continue = true;
        abstract_command* current = nullptr;

        while (*it != parser.end()) {
            should_continue = true;
            current = internal_parse(&should_continue, it, parser);

            garbage.push_back(current);
            commands.push_back(current);

            if (!should_continue) {
                break;
            }

            ++(*it);
        }

        if (commands.size() > 0) {
            operation = last()->operation;
        }
    }

    virtual abstract_command* internal_parse(bool* should_continue,
                                             shell_token_iterator* it, shell_parser parser) {
        (void)should_continue;
        (void)it;
        (void)parser;
        // Not implemented
        return nullptr;
    }

    abstract_command* last() {
        return commands[commands.size() - 1];
    }

    abstract_command* first() {
        return commands[0];
    }
};

struct pipeline {
    int in = 0;
    int out = 1;

    pipeline() {
        int fd[2];

        assert(pipe(fd) != -1);

        in = fd[0];
        out = fd[1];
    }

    void close_pipe() {
        close(in);
        close(out);
    }
};

// Represents a command pipeline, which contains a list of commands seperated by pipeline operators
struct command_list_pipeline : command_list {
    command_list_pipeline() : command_list() {}

    abstract_command* internal_parse(bool* should_continue, shell_token_iterator* it,
                                     shell_parser parser) {
        abstract_command* current = new command();
        current->parse(it, parser);

        if (it->type() != TYPE_PIPE && it->type() != TYPE_REDIRECT_OP) {
            *should_continue = false;
        }

        return current;
    }

    pid_t run() {
        std::vector<pipeline*> pipelines;
       
        pid_t pgid = -1;
        
        abstract_command* current;
        for (size_t index = 0; index < commands.size(); index++) {
            current = commands[index];

            if (is_background()) {
                current->flags |= BACKGROUND_FLAG;
            } 

            if (index != commands.size() - 1) {
                pipelines.push_back(new pipeline);
                current->out_fd = pipelines[index]->out;
            }

            if (index != 0) {
                current->in_fd = pipelines[index - 1]->in;
            }

            pid_t current_pid = current->run();
            
            // Create process group for first command
            if (!is_background()) {
                if (index == 0 && current_pid != 0) {
                    setpgid(current_pid, current_pid);
                    pgid = current_pid;
                }

                setpgid(current_pid, pgid);
            }

            current->close_pipes();
            current->command_exit();
        }

        // Close everything
        for (size_t index = 0; index < pipelines.size(); index++) {
            pipelines[index]->close_pipe();
            delete pipelines[index];
        }

        // Reclaim foreground for process group 
        if (!is_background()) {
            claim_foreground(pgid);
        }
        
        if (last()->pid >= 0) {
            assert(waitpid(last()->pid, &status, 0) > 0);
        }

        if (!is_background()) {
            claim_foreground(0);
        }

        return last()->pid;
    }
};

// Represents a conditional sequence, either || or &&
struct command_list_conditional : command_list {
    abstract_command* internal_parse(bool* should_continue, shell_token_iterator* it,
                                     shell_parser parser) {
        abstract_command* current = new command_list_pipeline();
        current->parse(it, parser);

        if (commands.size() == 0) {
            if (!current->is_conditional_header()) {
                *should_continue = false;
            }
        } else {
            if (current->operation != first()->operation) {
                *should_continue = false;
            }
        }

        if (it->type() == TYPE_BACKGROUND || it->type() == TYPE_SEQUENCE) {
            *should_continue = false;
        }

        return current;
    }

    pid_t run() {
        for (size_t index = 0; index < commands.size(); index++) {

            // Set background flag
            if (is_background()) {
                commands[index]->flags |= BACKGROUND_FLAG;
            }

            if (current_status == CANCELLED && !is_background()) {
                return -1;
            }

            if (index != 0) {
                // Determine if we should run current command based on previous command
                //  status
                if (commands[index - 1]->success()) {
                    if (first()->operation == TYPE_OR) {
                        commands[index]->status = commands[index - 1]->status;
                        continue;
                    }
                } else {
                    if (first()->operation == TYPE_AND) {
                        commands[index]->status = commands[index - 1]->status;
                        continue;
                    }
                }
            }

            // Run the command
            commands[index]->run();
        }

        return 0;
    }
};

// Represents a list of conditionals seperated by sequence or background operators
struct command_list_sequence : command_list {
    command_list_sequence() : command_list() {}

    abstract_command* internal_parse(bool* should_continue,
                                     shell_token_iterator* it, shell_parser parser) {
        (void)should_continue;

        abstract_command* current = new command_list_conditional();
        current->parse(it, parser);

        return current;
    }

    pid_t run() {
        for (size_t index = 0; index < commands.size(); index++) {
            // If not background command, wait for it to finish
            if (commands[index]->is_background()) {
                pid = fork();

                if (pid == 0) {
                    commands[index]->run();
                    command_exit();
                    return 0;
                }
            } else {
                // Run without forking
                commands[index]->run();
            }

            if (current_status == CANCELLED) {
                return -1;
            }
        }

        return 0;
    }
};

// run_list(c)
//    Run the command *list* starting at `c`. Initially this just calls
//    `c->run()` and `waitpid`; you’ll extend it to handle command lists,
//    conditionals, and pipelines.

void run_list(command_list_sequence* c) {
    c->run();

    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);

    clear_garbage();
}

// parse_line(s)
//    Parse the command list in `s` and return it. Returns `nullptr` if
//    `s` is empty (only spaces). You’ll extend it to handle more token
//    types.

command_list_sequence* parse_line(const char* s) {
    shell_parser parser(s);
    shell_token_iterator it = parser.begin();

    command_list_sequence* current = new command_list_sequence();
    current->parse(&it, parser);
    
    garbage.push_back(current);

    return current;
}

int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    bool quiet = false;

    // Check for `-q` option: be quiet (print no prompts)
    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = true;
        --argc, ++argv;
    }

    // Check for filename option: read commands from file
    if (argc > 1) {
        command_file = fopen(argv[1], "rb");
        if (!command_file) {
            perror(argv[1]);
            return 1;
        }
    }

    // - Put the shell into the foreground
    // - Ignore the SIGTTOU signal, which is sent when the shell is put back
    //   into the foreground
    claim_foreground(0);
    set_signal_handler(SIGTTOU, SIG_IGN);

    char buf[BUFSIZ];
    int bufpos = 0;
    bool needprompt = true;

    char path_buffer[100];

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
        if (needprompt && !quiet) {
            claim_foreground(0);

            // Add colors!!!
            printf("\033[34m");
            printf("[%d] ", getpid());
            printf("\033[32m");
            printf("%s ", getcwd(path_buffer, 100));
            printf("\033[0m");
            printf("$ ");

            fflush(stdout);
            needprompt = false;
        }

        // Read a string, checking for error or EOF
        if (fgets(&buf[bufpos], BUFSIZ - bufpos, command_file) == nullptr) {
            if (ferror(command_file) && errno == EINTR) {
                // ignore EINTR errors
                clearerr(command_file);
                buf[bufpos] = 0;
            } else {
                if (ferror(command_file)) {
                    perror("sh61");
                }
                break;
            }
        }

        // If a complete command line has been provided, run it
        bufpos = strlen(buf);
        if (bufpos == BUFSIZ - 1 || (bufpos > 0 && buf[bufpos - 1] == '\n')) {
            signal(SIGINT, signal_handler);
            current_status = NOT_CANCELLED;
            run_list(parse_line(buf));
            bufpos = 0;
            needprompt = 1;
        }
    }

    return 0;
}
