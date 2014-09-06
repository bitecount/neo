#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stack>
#include <vector>
#include <map>
#include <string>

using namespace std;

typedef enum
{
	OP_ADD = 0,
	OP_SUBTRACT = 1,
	OP_MULTIPLY = 2,
	OP_DIVIDE = 3,
	OP_MODULO = 4,
	OP_ASSIGN = 5,

	OP_BITWISE_AND = 6,
	OP_BITWISE_OR = 7,
	OP_BITWISE_XOR = 8,
	OP_BITWISE_NOT = 9,

	OP_CONSTANT = 10,
	OP_VARIABLE = 11,

	OP_OPEN_SCOPE = 12,
	OP_CLOSE_SCOPE = 13,

	OP_INVALID = 14,
	OP_EOF //Signifies the end of token stream.
} operator_t;

bool is_evaluation_operator(const operator_t& op)
{
	return ((op >= OP_ADD) && (op <= OP_BITWISE_NOT));
}

bool is_unary_operator(const operator_t& op)
{
	return (op == OP_BITWISE_NOT);
}

const char* operator_strings[] =
{
	"+",
	"-",
	"*",
	"/",
	"%",
	"=",

	"and",
	"or",
	"xor",
	"not",

	"CONSTANT",
	"VARIABLE",

	"(",
	")",

	"INVALID",
	"EOF"
};

typedef enum
{
	ERROR_UNDEFINED_VARIABLE = 0,
	ERROR_UNEXPECTED_TOKEN = 1,
	ERROR_ASSIGNMENT_TO_CONSTANT = 2,
	ERROR_UNEXPECTED_END_OF_EXPRESSION = 3,
	ERROR_BAD_EXPRESSION = 4,
	ERROR_PARSING_ERROR = 5
} error_type_t;

const char* error_codes[] =
{
	"Undefined variable used in expression",
	"Unexpected token encountered",
	"Assignment attempt to a constant",
	"Unexpected end of expression",
	"Improperly formed expression",
	"Parsing error"
};

typedef struct token
{
#define VARIABLE_NAME_LENGTH (31)
	operator_t type;
	union {
		int data; //data is valid only for OP_CONSTANT and varname is valid only for OP_VARIABLE.
		char varname[VARIABLE_NAME_LENGTH + 1]; 
		int error_code; //is valid only for OP_INVALID.
	};
	const char* str;
#undef VARIABLE_NAME_LENGTH
} token_t;

void print_token(const token_t& t)
{
	switch(t.type)
	{
		case OP_CONSTANT:
			printf("Token: type=%s data=%d\n", t.str, t.data);
			break;
		case OP_VARIABLE:
			printf("Token: type=%s varname=%s\n", t.str, t.varname);
			break;
		case OP_INVALID:
			printf("Token: type=%s error=%s\n", t.str, error_codes[t.error_code]);
			break;
		default:
			printf("Token: type=%s\n", t.str);
	} 
}

class SymbolTable
{
	public:
		bool get_symbol(const string& var, token_t& value);
		void set_symbol(const string& var, token_t value);

		void print_all_symbols();
	private:
		map < string, token_t > st;
};

bool SymbolTable::get_symbol(const string& var, token_t& value)
{
	map < string, token_t > :: iterator i = st.find(var);
	if(i == st.end())
		return false;
	else
	{
		value = (*i).second;
		return true;
	}
}

void SymbolTable::set_symbol(const string& var, token_t value)
{
	st[var] = value;
}

void SymbolTable::print_all_symbols()
{
	map < string, token_t > :: iterator i = st.begin(), j = st.end();	

	printf("Symbol Table <Begin>\n");
	while(i != j)
	{
		printf("%s = %d\n", (*i).first.c_str(), (*i).second.data);
		++i;
	}
	printf("Symbol Table <End>\n");
}

inline void set_operator_string(token_t* t)
{
	t->str = operator_strings[t->type];
}

/*
Read one token from istream and populate the token structure pointed by t.
Advance and return the incoming pointer so that it points to the next token in the stream.
*/
const char* getnext_token(const char* istream, token_t* t)
{
	if(istream == NULL) return NULL;
	t->type = OP_EOF;
	set_operator_string(t);

	//Skip white space
	while(*istream != '\0' && (*istream == ' ' || *istream == '\n' || *istream == '\t'))
		++istream;
	if(*istream == '\0') return NULL;

	t->type = OP_INVALID;
	t->data = ERROR_PARSING_ERROR;
	switch(*istream)
	{
		case '+' : t->type = OP_ADD; break;
		case '-' : t->type = OP_SUBTRACT; break;
		case '*' : t->type = OP_MULTIPLY; break;
		case '/' : t->type = OP_DIVIDE; break;
		case '%' : t->type = OP_MODULO; break;	
		case '=' : t->type = OP_ASSIGN; break;
		case '&' : t->type = OP_BITWISE_AND; break;
		case '|' : t->type = OP_BITWISE_OR; break;
		case '^' : t->type = OP_BITWISE_XOR; break;
		case '~' : t->type = OP_BITWISE_NOT; break;
		case '(' : t->type = OP_OPEN_SCOPE; break;
		case ')' : t->type = OP_CLOSE_SCOPE; break;
	}
	if((*istream >= '0' && *istream <= '9') || (*istream == '-'))
	{
		int accumulate = 0;
		bool negative = false;
		const char* r = istream;

		//Ignore negative sign followed by a character which isn't a digit.
		if(r[0] == '-' && (r[1] < '0' || r[1] > '9'))
			goto skip_reading_literal; 

		if(r[0] == '-')
		{
			negative = true; 
			++r;
		}
		t->type = OP_CONSTANT;
		while(*r >= '0' && *r <= '9')
		{
			accumulate = (accumulate * 10) + (*r - '0');
			r++;
		}	
		t->data = (negative ? -accumulate : accumulate);
		istream = --r;
	}
skip_reading_literal:
	if(*istream >= 'a' && *istream <= 'z')
	{
		const char* r = istream;
		int i = 0;
		t->type = OP_VARIABLE;
		while(*r >= 'a' && *r <= 'z' && *r != '\0')
		{
			t->varname[i++] = *r;
			r++;
		}
		t->varname[i] = '\0';
		istream = --r;
	}

	set_operator_string(t);
	
	//Consume exactly one token from istream and return a pointer from where the next token starts.
	return ++istream;
}

/*
Evaluate the well formed postfix expression in the vector v, and populate the result in 'result'.
*/
token_t evaluate_postfix(const vector< token_t > &v, stack< token_t> &s, SymbolTable& st)
{
#define DO_VARIABLE_LOOKUP(token) do { \
	if(token.type == OP_VARIABLE) \
	{ \
		token_t newtoken; \
		if(!st.get_symbol(string(token.varname), newtoken)) \
		{ \
			err.error_code = ERROR_UNDEFINED_VARIABLE; \
			return err; \
		} \
		token = newtoken; \
	} \
} while(0)

#define RETURN_IF_EMPTY do { \
	if(s.empty()) \
	{ \
		err.error_code = ERROR_UNEXPECTED_END_OF_EXPRESSION; \
		return err; \
	} \
} while(0)

	token_t err;
	err.type = OP_INVALID;
	set_operator_string(&err);

	for(int i = 0; i < v.size(); ++i)
	{
		if(v[i].type == OP_CONSTANT || v[i].type == OP_VARIABLE)
			s.push(v[i]);
		else if(is_evaluation_operator(v[i].type))
		{
			//Evaluate the expression (operator op) for unary operators.
			if(is_unary_operator(v[i].type))
			{
				RETURN_IF_EMPTY;
				token_t op = s.top();
				s.pop();
				DO_VARIABLE_LOOKUP(op);
				switch(v[i].type)
				{
					case OP_BITWISE_NOT: op.data = ~op.data; break;
				}
				s.push(op);
				continue;
			}

			//Evaluate the expression (op1 operator op2) for a binary operators.
			RETURN_IF_EMPTY;
			token_t op2 = s.top();
			s.pop();
			RETURN_IF_EMPTY;
			token_t op1 = s.top();
			s.pop();
			
			//Lookup op1 and op2. If operator is OP_ASSIGN, then op1 should be a variable and shouldn't be looked up.
			if(v[i].type != OP_ASSIGN)
				DO_VARIABLE_LOOKUP(op1);
			DO_VARIABLE_LOOKUP(op2);

			switch(v[i].type)
			{
				case OP_ADD: op1.data += op2.data; break;
				case OP_SUBTRACT: op1.data -= op2.data; break;
				case OP_MULTIPLY: op1.data *= op2.data; break;
				case OP_DIVIDE: op1.data /= op2.data; break;
				case OP_MODULO: op1.data %= op2.data; break;
				case OP_BITWISE_AND: op1.data &= op2.data; break;
				case OP_BITWISE_OR: op1.data |= op2.data; break;
				case OP_BITWISE_XOR: op1.data ^= op2.data; break;
				case OP_ASSIGN:
				//op1 should be a variable, otherwise generate an error.
				if(op1.type != OP_VARIABLE)
				{
					err.error_code = ERROR_ASSIGNMENT_TO_CONSTANT;
					return err;
				}
				string lvalue(op1.varname);
				st.set_symbol(lvalue, op2);
			}
			s.push(op1);
		}
		else
		{
			err.error_code = ERROR_BAD_EXPRESSION;
			return err;
		}
	}
	
	//The evaluation is done. If the stack is empty, report an error. Otherwise if it contains a variable name
	//do a lookup.
	RETURN_IF_EMPTY;
	token_t res = s.top();
	s.pop();
	if(res.type == OP_VARIABLE)
		DO_VARIABLE_LOOKUP(res);
	return res;

#undef POP_AND_RETURN_ON_ERROR
#undef DO_VARIABLE_LOOKUP
}

/*
Assign a priority value for each operator.
*/
int priority_value(operator_t x)
{
	switch(x)
	{
		case OP_ADD :
		case OP_SUBTRACT : return 0;
		
		case OP_MULTIPLY :
		case OP_DIVIDE : return 5;

		case OP_BITWISE_AND:
		case OP_BITWISE_OR:
		case OP_BITWISE_XOR: return 0;
		case OP_BITWISE_NOT: return 10;

		case OP_ASSIGN : return -5;
	}
	return 0;
}

/*
Return
1: If operator x is of higher priority than operator y.
0: x and y are of same priority.
-1: If operator y is of higher priority than operator x.
*/
int is_higher_priority(operator_t x, operator_t y)
{
	int i = priority_value(x) - priority_value(y);

	//Operators such as = have to be evaluated from right to left.
	if(x == OP_ASSIGN && y == OP_ASSIGN)
		return -1;

	return (i > 0) ? 1 : (i == 0 ? 0 : -1);
}

/*
Evaluate the infix expression pointed by p.
*/
token_t evaluate_infix(const char* p, SymbolTable& st)
{
#define POP_ALL do { \
	while(!s.empty()) \
	{ \
		token_t top = s.top(); \
		if(top.type != OP_OPEN_SCOPE) \
			v.push_back(top); \
		s.pop(); \
	} \
} while(0)

#define POP_AND_POPULATE_VECTOR do { \
	while(!s.empty()) \
	{ \
		token_t top = s.top(); \
		if(top.type == OP_OPEN_SCOPE) break; \
		v.push_back(top); \
		s.pop(); \
	} \
} while(0)

#define POP_HIGH_PRIORITY_AND_POPULATE_VECTOR do { \
	while(!s.empty()) \
	{ \
		token_t top = s.top(); \
		if(top.type == OP_OPEN_SCOPE) break; \
		if(is_higher_priority(top.type, t.type) < 0) break; \
		v.push_back(top); \
		s.pop(); \
	} \
} while(0)

	stack < token_t > s; //Used for conversion from infix to postfix.
	vector< token_t > v; //Vector that stores the postfix expression.

	token_t t;
	const char* q;

	t.type = OP_INVALID;
	if(p == NULL) return t;

	while(p)
	{
		q = getnext_token(p, &t);
		
		switch(t.type)
		{
			case OP_CONSTANT:
			case OP_VARIABLE:
				v.push_back(t);
				break;
			case OP_ADD:
			case OP_SUBTRACT:
			case OP_MULTIPLY:
			case OP_DIVIDE:
			case OP_MODULO:
			case OP_ASSIGN:
			case OP_BITWISE_AND:
			case OP_BITWISE_OR:
			case OP_BITWISE_XOR:
			case OP_BITWISE_NOT:
			//Incoming operator; pop operators from the stack which are of higher priority, put them into the vector and then push
			//this element into the stack.
				POP_HIGH_PRIORITY_AND_POPULATE_VECTOR;
				s.push(t); break;
			case OP_OPEN_SCOPE:
				s.push(t); break;
			case OP_CLOSE_SCOPE:
			//Pop operators from the stack and put them into the vector until an OP_OPEN_SCOPE type is removed.
				POP_AND_POPULATE_VECTOR;
				break;
			case OP_EOF: goto evaluate_expression;
			case OP_INVALID: return t;
		}
		p = q;
	}
evaluate_expression:
	POP_ALL;

	return evaluate_postfix(v, s, st);

#undef POP_HIGH_PRIORITY_AND_POPULATE_VECTOR
#undef POP_AND_POPULATE_VECTOR
#undef POP_ALL
}

int main()
{
	char buffer[128];
	SymbolTable st;

	printf(">> ");
	while(gets(buffer))
	{
		if(!strcmp(buffer, "quit"))
			break;
		
		token_t t = evaluate_infix(buffer, st);		
		print_token(t);
		printf(">> ");
	}
	return 0;
}
