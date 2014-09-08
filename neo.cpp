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

	OP_OBJECT = 10,
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
	ERROR_PARSING_ERROR = 5,
	ERROR_UNDEFINED_OPERATOR = 6
} error_type_t;

const char* error_codes[] =
{
	"Undefined variable used in expression",
	"Unexpected token encountered",
	"Assignment attempt to a constant",
	"Unexpected end of expression",
	"Improperly formed expression",
	"Parsing error",
	"Operator undefined"
};

typedef enum
{
	OBJECT_INTEGER = 0,
	OBJECT_FLOAT,
	OBJECT_STRING	
} object_type_t;

const char* object_type_strings[] =
{
	"Integer",
	"Float",
	"String"
};

typedef void* object_handle_t;

class object
{
	public:
		static object* create_object(int v);
		static object* create_object(double v);
		static object* create_object(const char* s);

		static void object_reap(object* o);

		static void debug_string(const object* o, char* buffer, int bufferlength);

		void increment_refcount();
		void decrement_refcount();

		void print_object(bool verbose = false);
		int get_object_count() { return object_count; }

#define PROTOTYPE_OPERATOR_FUNCTION(op) \
		friend object* operator op (object& lhs, object& rhs);

		PROTOTYPE_OPERATOR_FUNCTION(+)
		PROTOTYPE_OPERATOR_FUNCTION(-)
		PROTOTYPE_OPERATOR_FUNCTION(*)
		PROTOTYPE_OPERATOR_FUNCTION(/)
		PROTOTYPE_OPERATOR_FUNCTION(%)
		PROTOTYPE_OPERATOR_FUNCTION(&)
		PROTOTYPE_OPERATOR_FUNCTION(|)
		PROTOTYPE_OPERATOR_FUNCTION(^)

#undef  PROTOTYPE_OPERATOR_FUNCTION

		friend object* operator ~ (object& rhs);
	private:
		object_type_t type;
		union {
			object_handle_t handle;
			int intvalue;
			double floatvalue;
		};
		int refcount;

		static int object_count;

		object() : refcount(0) { ++object_count; }

		object(int v) : type(OBJECT_INTEGER), intvalue(v), refcount(0) { ++object_count; }
		object(double v) : type(OBJECT_FLOAT), floatvalue(v), refcount(0) { ++object_count; }
		object(const char* s) : type(OBJECT_STRING), refcount(0)
		{
			this->handle = new char[ strlen(s) + 1 ];
			strcpy((char*) this->handle, s);
			++object_count;
		}

		~object() {}

		double object_to_double()
		{
			return (type == OBJECT_INTEGER) ? (double) intvalue : (type == OBJECT_FLOAT ? floatvalue : (double)0 );
		}
};

typedef object* object_pointer_t;

//Initialize static members of the class object.
int object::object_count = 0;

object* object::create_object(int v)
{
	return new object(v);
}

object* object::create_object(double v)
{
	return new object(v);
}

object* object::create_object(const char* s)
{
	return new object(s);
}

void object::increment_refcount()
{
	++refcount;
}

void object::decrement_refcount()
{
	if(--refcount == 0)
		object_reap(this);
}

void object::object_reap(object* o)
{
	if(o->refcount == 0)
	{
		if(o->type == OBJECT_STRING)
			delete [] ((char*) o->handle);
		delete o;
	}
}

void object::debug_string(const object* o, char* buffer, int bufferlength)
{
	switch(o->type)
	{
		case OBJECT_STRING:
		if(bufferlength)
		{
			strncpy(buffer, (const char*) o->handle, bufferlength);
			buffer[bufferlength - 1] = '\0';
		}
		break;

		case OBJECT_INTEGER:
		if(bufferlength)
		{
			snprintf(buffer, bufferlength, "%d", o->intvalue);
			buffer[bufferlength - 1] = '\0';
		}
		break;

		case OBJECT_FLOAT:
		if(bufferlength)
		{
			snprintf(buffer, bufferlength, "%.2f", o->floatvalue);
			buffer[bufferlength - 1] = '\0';
		}
		break;
	}
}

void object::print_object(bool verbose)
{
	if(verbose)
	{
		printf("object@%p\n", this);
		printf("type=%s reference_count=%d ", object_type_strings[this->type], this->refcount);
	}
	switch(this->type)
	{
		case OBJECT_INTEGER: printf("[%d]", this->intvalue); break;
		case OBJECT_FLOAT:   printf("[%.2f]", this->floatvalue); break;
		case OBJECT_STRING:  printf("['%s'] length=%ld", (const char*)this->handle, strlen((const char*)this->handle)); break;
	}
	printf("\n");
}

#define OPERATOR_FUNCTION(op) \
object* operator op (object& lhs, object& rhs) \
{ \
	object* result = NULL; \
	if(lhs.type == OBJECT_INTEGER && rhs.type == OBJECT_INTEGER) \
	{ \
		result = object::create_object(lhs.intvalue op rhs.intvalue); \
	} \
	return result; \
}

#define OPERATOR_INTEGER_FLOAT_FUNCTION(op) \
object* operator op (object& lhs, object& rhs) \
{ \
	object* result = NULL; \
	if(lhs.type == OBJECT_INTEGER && rhs.type == OBJECT_INTEGER) \
		result = object::create_object(lhs.intvalue op rhs.intvalue); \
	else if(#op == "+" && lhs.type == OBJECT_STRING && rhs.type == OBJECT_STRING) \
	{ \
		/*For efficiency, create the object using private constructor and set up type and handle.*/ \
		result = new object(); \
		result->type = OBJECT_STRING; \
		result->handle = new char [strlen((const char*)lhs.handle) + strlen((const char*)rhs.handle) + 1]; \
		strcpy((char*) result->handle, (const char*)lhs.handle); \
		strcpy(((char*) result->handle) + strlen((const char*)result->handle), (const char*)rhs.handle); \
	} \
	else if((lhs.type == OBJECT_INTEGER || lhs.type == OBJECT_FLOAT) && \
		(rhs.type == OBJECT_INTEGER || rhs.type == OBJECT_FLOAT)) \
		result = object::create_object(lhs.object_to_double() op rhs.object_to_double()); \
\
	return result; \
}

object* operator % (object& lhs, object& rhs)
{
	object* result = NULL;
	if(lhs.type == OBJECT_INTEGER && rhs.type == OBJECT_INTEGER)
		result = object::create_object(lhs.intvalue % rhs.intvalue);
	//% is undefined for floating point values in C. However neo defines % analogous to how it operates for an integer.
	else if((lhs.type == OBJECT_INTEGER || lhs.type == OBJECT_FLOAT) &&
		(rhs.type == OBJECT_INTEGER || rhs.type == OBJECT_FLOAT))
	{
		double l = lhs.object_to_double(), r = rhs.object_to_double();
		result = object::create_object(l - ((long)(l / r) * r));
	}

	return result;
}

OPERATOR_INTEGER_FLOAT_FUNCTION(+)
OPERATOR_INTEGER_FLOAT_FUNCTION(-)
OPERATOR_INTEGER_FLOAT_FUNCTION(*)
OPERATOR_INTEGER_FLOAT_FUNCTION(/)
OPERATOR_FUNCTION(&)
OPERATOR_FUNCTION(|)
OPERATOR_FUNCTION(^)

#undef OPERATOR_INTEGER_FLOAT_FUNCTION
#undef OPERATOR_FUNCTION

object* operator ~ (object& rhs)
{
	object* result = NULL;
	if(rhs.type == OBJECT_INTEGER)
	{
		result = object::create_object(~rhs.intvalue);
	}
	return result;
}

class token
{
	public:
#define VARIABLE_NAME_LENGTH (31)
	operator_t type;
	union {
		object_pointer_t data; 			//valid only for OP_OBJECT.
		char varname[VARIABLE_NAME_LENGTH + 1]; //valid only for OP_VARIABLE.
		int error_code; 			//valid only for OP_INVALID.
	};

	token() { data = NULL; }
#undef VARIABLE_NAME_LENGTH
};
typedef token token_t;

void print_token(const token_t& t)
{
	switch(t.type)
	{
		case OP_OBJECT:
			printf("token: type=%s data=%p\n", operator_strings[t.type], t.data);
			break;
		case OP_VARIABLE:
			printf("token: type=%s varname=%s\n", operator_strings[t.type], t.varname);
			break;
		case OP_INVALID:
			printf("token: type=%s error=%s\n", operator_strings[t.type], error_codes[t.error_code]);
			break;
		default:
			printf("token: type=%s\n", operator_strings[t.type]);
	} 
}

class symboltable
{
	public:
		bool get_symbol(const string& var, object_pointer_t& value);
		void set_symbol(const string& var, object_pointer_t value);

		void print_all_symbols();
	private:
		map < string, object_pointer_t > st;
};

bool symboltable::get_symbol(const string& var, object_pointer_t& value)
{
	map < string, object_pointer_t > :: iterator i = st.find(var);
	if(i == st.end())
		return false;
	else
	{
		value = (*i).second;
		return true;
	}
}

void symboltable::set_symbol(const string& var, object_pointer_t value)
{
	st[var] = value;
}

void symboltable::print_all_symbols()
{
	map < string, object_pointer_t > :: iterator i = st.begin(), j = st.end();	

	printf("symbol table <begin>\n");
	while(i != j)
	{
		printf("%s = %p\n", (*i).first.c_str(), (*i).second);
		++i;
	}
	printf("symbol Table <end>\n");
}

/*
Read one token from istream and populate the token structure pointed by t.
Advance and return the incoming pointer so that it points to the next token in the stream.
*/
const char* getnext_token(const char* istream, token_t* t)
{
	if(istream == NULL) return NULL;
	t->type = OP_EOF;

	//Skip white space.
	while(*istream != '\0' && (*istream == ' ' || *istream == '\n' || *istream == '\t'))
		++istream;
	if(*istream == '\0') return NULL;

	t->type = OP_INVALID;
	t->error_code = ERROR_PARSING_ERROR;

	//Look for operators.
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

	//Look for an integer literal.
	if(isdigit(*istream) || (*istream == '-' && isdigit(istream[1])))
	{
		char * r = NULL;
		double v = strtod(istream, &r);
		int v_int = (int) v;

		t->type = OP_OBJECT;
		if(v - v_int == 0)
			t->data = object::create_object(v_int);
		else
			t->data = object::create_object(v);

		if(r == istream)
			istream = istream + strlen(istream); 
		else
			istream = --r;
	}

	//Look for a string literal (enclosed in '').
	if(*istream == '\'')
	{
		const char* r = istream + 1;
		char buffer[64] = { 0 };
		int k = 0;
		while(*r != '\0' && *r != '\'')
		{
			buffer[k++] = *r;
			++r;
		}
		if(*r == '\0')
			goto skip_reading_literal;
		else
		{
			buffer[k] = '\0';
			t->type = OP_OBJECT;
			t->data = object::create_object((const char*) buffer);
			istream = r;
		}
	}
	
skip_reading_literal:
	//Look for a variable name
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
	
	//Consume exactly one token from istream and return a pointer from where the next token starts.
	return ++istream;
}

/*
Evaluate the well formed postfix expression in the vector v, and populate the result in 'result'.
*/
token_t evaluate_postfix(const vector< token_t > &v, stack< token_t> &s, symboltable& st)
{
#define GET_OBJECT_POINTER(token, object_pointer, reporterror) do { \
	if(token.type == OP_VARIABLE) \
	{ \
		if(!st.get_symbol(string(token.varname), object_pointer) && reporterror) \
		{ \
			err.error_code = ERROR_UNDEFINED_VARIABLE; \
			goto cleanup_and_return_error; \
		} \
	} \
	else if(token.type == OP_OBJECT) \
		object_pointer = token.data; \
} while(0)

#define RETURN_IF_NULL(x) do { \
	if(x == NULL) \
	{ \
		err.error_code = ERROR_UNDEFINED_OPERATOR; \
		goto cleanup_and_return_error; \
	} \
} while(0)

#define RETURN_IF_EMPTY do { \
	if(s.empty()) \
	{ \
		err.error_code = ERROR_UNEXPECTED_END_OF_EXPRESSION; \
		goto cleanup_and_return_error; \
	} \
} while(0)

	token_t err;
	err.type = OP_INVALID;

	int i, j;
	for(i = 0; i < v.size(); ++i)
	{
		if(v[i].type == OP_OBJECT || v[i].type == OP_VARIABLE)
			s.push(v[i]);
		else if(is_evaluation_operator(v[i].type))
		{
			//Evaluate the expression (operator op) for unary operators.
			if(is_unary_operator(v[i].type))
			{
				RETURN_IF_EMPTY;
				token_t op = s.top();
				s.pop();
				object_pointer_t p = NULL, r;
				GET_OBJECT_POINTER(op, p, true);
				switch(v[i].type)
				{
					case OP_BITWISE_NOT: r = ~(*p); break;
				}
				token_t result;
				result.type = OP_OBJECT;
				RETURN_IF_NULL(r);
				result.data = r;
				s.push(result);
				if(op.type == OP_OBJECT) object::object_reap(p);
				continue;
			}

			//Evaluate the expression (op1 operator op2) for a binary operators.
			RETURN_IF_EMPTY;
			token_t op2 = s.top();
			s.pop();
			RETURN_IF_EMPTY;
			token_t op1 = s.top();
			s.pop();
			
			object_pointer_t p1 = NULL, p2 = NULL, r = NULL;

			//For assignment op1 need not already be a defined variable.
			if(v[i].type == OP_ASSIGN)
				GET_OBJECT_POINTER(op1, p1, false);
			else
				GET_OBJECT_POINTER(op1, p1, true);

			GET_OBJECT_POINTER(op2, p2, true);

			switch(v[i].type)
			{
				case OP_ADD: 		r = *p1 + *p2 ; break;
				case OP_SUBTRACT: 	r = *p1 - *p2 ; break;
				case OP_MULTIPLY: 	r = *p1 * *p2 ; break;
				case OP_DIVIDE: 	r = *p1 / *p2 ; break;
				case OP_MODULO: 	r = *p1 % *p2 ; break;
				case OP_BITWISE_AND: 	r = *p1 & *p2 ; break;
				case OP_BITWISE_OR: 	r = *p1 | *p2 ; break;
				case OP_BITWISE_XOR: 	r = *p1 ^ *p2 ; break;
				case OP_ASSIGN:
				//op1 should be a variable, otherwise generate an error.
				if(op1.type != OP_VARIABLE)
				{
					err.error_code = ERROR_ASSIGNMENT_TO_CONSTANT;
					s.push(op1); s.push(op2);			
					goto cleanup_and_return_error;
				}
				string lvalue(op1.varname);
				st.set_symbol(lvalue, p2);
				if(p2) p2->increment_refcount();
				if(p1) p1->decrement_refcount();
				r = p2;
			}
			token_t result;
			result.type = OP_OBJECT;
			RETURN_IF_NULL(r);
			result.data = r;
			s.push(result);

			//If objects used in the expression are constants, we can release them.
			if(v[i].type != OP_ASSIGN)
			{
				if(op1.type == OP_OBJECT) object::object_reap(p1);
				if(op2.type == OP_OBJECT) object::object_reap(p2);
			}
		}
		else
		{
			err.error_code = ERROR_BAD_EXPRESSION;
			goto cleanup_and_return_error;
		}
		j = i; //Processing is complete upto j. In case of error in next iteration, the cleanup code examines the vector from j.
	}
	
	//The stack should have had exactly one element after completion of evaluation.
	if(s.size() != 1)
		err.error_code = ERROR_BAD_EXPRESSION;
	else
	{
		token_t res = s.top(); //No errors detected during evaluation.
		s.pop();
		object_pointer_t p = NULL;
		GET_OBJECT_POINTER(res, p, true);
		res.type = OP_OBJECT;
		res.data = p;
		return res;
	}
	
cleanup_and_return_error:
	for( ++j ; j < v.size(); ++j)
	{
		if(v[j].type == OP_OBJECT)
		{
			object_pointer_t p = NULL;
			GET_OBJECT_POINTER(v[j], p, true);
			object::object_reap(p);
		}
	}
	while(!s.empty())
	{
		token_t t = s.top();
		s.pop();
		if(t.type == OP_OBJECT)
		{
			object_pointer_t p = NULL;
			GET_OBJECT_POINTER(t, p, true);
			object::object_reap(p);
		}
	}

	return err;

#undef RETURN_IF_EMPTY
#undef RETURN_IF_NULL
#undef GET_OBJECT_POINTER
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
token_t evaluate_infix(const char* p, symboltable& st)
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
			case OP_OBJECT:
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

void run_testcases_from_file(FILE* file, symboltable& st)
{
	char expr[128], expected_result[128], result[128];
	int i = 0, p = 0;
	
#define REMOVE_TRAILING_NEWLINE(s) do { \
if(s[strlen(s) - 1] == '\n') \
	s[strlen(s) - 1] = '\0'; \
} while(0)
	
	while(fgets(expr, sizeof(expr), file))
	{
		REMOVE_TRAILING_NEWLINE(expr);
		
		if(!strcmp(expr, "quit"))
			break;
		token_t t = evaluate_infix(expr, st);
		fgets(expected_result, sizeof(expected_result), file);

		REMOVE_TRAILING_NEWLINE(expected_result);

		if(t.type == OP_OBJECT && t.data)
		{
			object::debug_string(t.data, result, sizeof(result));

			if(!strcmp(expected_result, result))
				++p, printf("test case [%s] *PASS*\n", expr);
			else
				printf("test case [%s] expected [%s] obtained [%s] *FAIL*\n", expr, expected_result, result);
		}
		++i;		
	}
	printf("total test cases=%d passed=%d failed=%d\n", i, p, i-p);

#undef REMOVE_TRAILING_NEWLINE
}

const char prompt[] = "neo] ";

int main(int argv, char** argc)
{
	char buffer[128];
	symboltable st;

	FILE* file = NULL;

	if(argv == 2)
	{
		file = fopen(argc[1], "r");
		if(file == NULL)
		{
			printf("cannot open file %s\n", argc[1]);
			return -1;
		}
		run_testcases_from_file(file, st);
		fclose(file);
	}
	else
	{
		printf("%s", prompt);
		while(gets(buffer))
		{
			if(!strcmp(buffer, "quit"))
				break;
		
			token_t t = evaluate_infix(buffer, st);
			if(t.type == OP_OBJECT && t.data)		
				t.data->print_object();
			else
				print_token(t);
			printf("%s", prompt);
		}
		printf("\n");
	}

	return 0;
}
