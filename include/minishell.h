#ifndef MINISHELL_H
# define MINISHELL_H

# include <signal.h>
# include <stddef.h>
# include <sys/types.h>

# define PROMPT "minishell$ "

/* ------------------------------------------------------------- tokens */
typedef enum e_tok
{
	T_WORD,
	T_PIPE,
	T_AND,
	T_OR,
	T_LPAREN,
	T_RPAREN,
	T_IN,
	T_OUT,
	T_APPEND,
	T_HEREDOC,
	T_END
}	t_tok;

typedef struct s_token
{
	t_tok			type;
	char			*text;
	struct s_token	*next;
}	t_token;

/* ------------------------------------------------------------- syntax tree */
typedef struct s_word
{
	char			*text;
	struct s_word	*next;
}	t_word;

/* word: file name, or the here-document delimiter as typed (quotes kept)
** fd: the here-document's text, ready to be read (-1 for other kinds) */
typedef struct s_redir
{
	t_tok			type;
	char			*word;
	int				fd;
	struct s_redir	*next;
}	t_redir;

typedef enum e_kind
{
	N_CMD,
	N_SUBSHELL,
	N_PIPE,
	N_AND,
	N_OR
}	t_kind;

/*
** N_CMD       words + redirs
** N_SUBSHELL  left = the list inside ( ), redirs apply to all of it
** N_PIPE      left = first command, the others follow through next
** N_AND/N_OR  left && right, left || right
*/
typedef struct s_node
{
	t_kind			kind;
	t_word			*words;
	t_redir			*redirs;
	struct s_node	*left;
	struct s_node	*right;
	struct s_node	*next;
}	t_node;

/* ------------------------------------------------------------- shell */
typedef struct s_env
{
	char			*key;
	char			*value;
	struct s_env	*next;
}	t_env;

typedef struct s_shell
{
	t_env	*env;
	int		status;
	int		interactive;
	int		in_child;
	int		interrupted;
	int		should_exit;
	char	*line;
	t_node	*tree;
}	t_shell;

/* expansion buffer: every character remembers whether it was quoted */
typedef struct s_buf
{
	char	*s;
	char	*q;
	size_t	len;
	size_t	cap;
}	t_buf;

/* the only global: the number of the last signal received */
extern volatile sig_atomic_t	g_signal;

/* main.c */
void	child_exit(t_shell *sh, int status);

/* input.c */
char	*read_input(t_shell *sh, const char *prompt);

/* signals.c */
void	signals_prompt(void);
void	signals_heredoc(void);
void	signals_ignore(void);
void	signals_default(void);

/* lexer.c */
t_token	*lex(const char *line, int *unclosed);
void	free_tokens(t_token *tok);

/* parser.c, tree.c */
t_node	*parse(t_shell *sh, t_token *tokens);
t_node	*new_node(t_kind kind);
void	add_word(t_word **list, char *text);
void	add_redir(t_redir **list, t_tok type, char *word);
void	free_tree(t_node *node);

/* heredoc.c */
int		prepare_heredocs(t_shell *sh, t_node *node);

/* buf.c */
void	buf_push(t_buf *b, char c, char quoted);
void	buf_push_str(t_buf *b, const char *s, char quoted);
char	*buf_take(t_buf *b);

/* expand.c, expand_dollar.c, fields.c, wildcard.c */
size_t	expand_dollar(t_shell *sh, const char *s, t_buf *b, int in_quotes);
t_word	*expand_word(t_shell *sh, const char *raw);
char	**expand_args(t_shell *sh, t_word *words);
int		expand_redir_word(t_shell *sh, const char *raw, char **out);
char	*expand_heredoc_line(t_shell *sh, char *line);
char	*remove_quotes(const char *raw);
t_word	*split_fields(t_buf *b, int had_quotes);
t_word	*glob_field(const char *s, const char *q);

/* redirect.c */
int		apply_redirs(t_shell *sh, t_redir *r);

/* exec.c, pipeline.c, external.c, path.c, wait.c */
int		exec_node(t_shell *sh, t_node *node);
int		exec_pipeline(t_shell *sh, t_node *first);
int		run_command(t_shell *sh, t_node *cmd);
void	exec_external(t_shell *sh, char **argv);
char	*find_command(t_shell *sh, char *name, int *code);
int		wait_children(t_shell *sh, pid_t last);

/* builtins */
int		is_builtin(const char *name);
int		run_builtin(t_shell *sh, char **argv);
int		builtin_echo(char **argv);
int		builtin_pwd(t_shell *sh);
int		builtin_env(t_shell *sh, char **argv);
int		builtin_cd(t_shell *sh, char **argv);
int		builtin_export(t_shell *sh, char **argv);
int		builtin_unset(t_shell *sh, char **argv);
int		builtin_exit(t_shell *sh, char **argv);
int		valid_identifier(const char *s, size_t len);

/* env.c, env_list.c */
t_env	*env_init(char **envp);
t_env	*env_find(t_env *env, const char *key);
char	*env_get(t_env *env, const char *key);
void	env_set(t_env **env, const char *key, const char *value);
void	env_unset(t_env **env, const char *key);
char	**env_to_array(t_env *env);
void	env_free(t_env *env);

/* str.c, str2.c, mem.c, error.c */
size_t	ft_strlen(const char *s);
char	*ft_strdup(const char *s);
char	*ft_strndup(const char *s, size_t n);
char	*ft_strjoin(const char *a, const char *b);
char	*ft_strjoin3(const char *a, const char *b, const char *c);
int		ft_strcmp(const char *a, const char *b);
int		ft_strncmp(const char *a, const char *b, size_t n);
char	*ft_strchr(const char *s, int c);
int		ft_isalpha(int c);
int		ft_isdigit(int c);
int		ft_isalnum(int c);
int		is_blank(int c);
char	*ft_itoa(long n);
void	*xmalloc(size_t size);
void	free_strs(char **strs);
void	put_str(int fd, const char *s);
void	print_error(const char *who, const char *what);
void	print_error3(const char *a, const char *b, const char *c);

#endif
