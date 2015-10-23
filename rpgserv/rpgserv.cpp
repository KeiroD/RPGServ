#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stack>
#include <utility>
#include <cctype>
#include <limits.h>
#ifdef _WIN32
# include <float.h>
#endif
#include <cmath>
#include "hashcomp.h"
#ifdef tolower
# undef tolower
#endif
#ifdef max
# undef max
#endif
#ifdef min
# undef min
#endif

static const int DICE_MAX_TIMES = 25;
static const unsigned DICE_MAX_DICE = 99999;
static const unsigned DICE_MAX_SIDES = 99999;
enum DICE_ERRS /* Error codes */
{
	DICE_ERR_NONE,
	DICE_ERR_PARSE,
	DICE_ERR_DIV0,
	DICE_ERR_UNDEFINED,
	DICE_ERR_UNACCEPT_DICE,
	DICE_ERR_UNACCEPT_SIDES,
	DICE_ERR_OVERUNDERFLOW,
	DICE_ERR_STACK
};
enum DICE_TYPES /* Roll types */
{
	DICE_TYPE_ROLL,
	DICE_TYPE_CALC,
	DICE_TYPE_EXROLL,
	DICE_TYPE_EXCALC,
	DICE_TYPE_DND3E,
	DICE_TYPE_EARTHDAWN
};



static DICE_ERRS DiceErrCode;  /* Error code, see DICE_ERRS above */
static int	DiceErrNum;    /* Value that caused last error, if needed */
static unsigned	DiceErrPos;    /* Position of last error, if needed */
static Anope::string	DiceExOutput,      /* Extended output string */
			DiceShortExOutput, /* Shorter extended output string */
			RollSource,        /* Source of the roll when SourceIsBot is set */
			DiceErrStr;        /* Error string */


int ran(int k) 
{
	double x = RAND_MAX + 1.0;
	int y;          
	y = 1 + rand() * (k / x);
	return ( y );
}

int Dice(int many, unsigned sides)
{
	int i,q=0;
	if(many>10000)
		many=10000;
	if(sides>1000000)
		sides=1000000;
	for (i=0;i<many;i++) {
		q += ran(sides);;
	}
	return ( q );
}

 static const int MERS_N = 624;
 static const int MERS_M = 397;
 static const int MERS_R = 31;
 static const int MERS_U = 11;
 static const int MERS_S = 7;
 static const int MERS_T = 15;
 static const int MERS_L = 18;
 static const int MERS_A = 0x9908B0DF;
 static const int MERS_B = 0x9D2C5680;
 static const int MERS_C = 0xEFC60000;

 static int mt[MERS_N]; /* State vector */
 static int mti; /* Index into mt */


/** Determine if the double-precision floating point value is infinite or not.
 * @param num The double-precision floating point value to check
 * @return true if the value is infinite, false otherwise
 */
static bool is_infinite(double num)
{
#ifdef _WIN32
	int fpc = _fpclass(num);
	return fpc == _FPCLASS_NINF || fpc == _FPCLASS_PINF;
#else
	return std::isinf(num);
#endif
}

/** Determine if the double-precision floating point value is NaN (Not a Number) or not.
 * @param num The double-precision floating point value to check
 * @return true if the value is NaN, false otherwise
 */
static bool is_notanumber(double num)
{
#ifdef _WIN32
	int fpc = _fpclass(num);
	return fpc == _FPCLASS_SNAN || fpc == _FPCLASS_QNAN;
#else
	return std::isnan(num);
#endif
}

#ifdef _WIN32
/** Calculate inverse hyperbolic cosine for Windows.
 * @param num The double-precision floating point value to calculate
 * @return The inverse hyperbolic cosine of the value
 */
static double acosh(double num)
{
	return log(num + sqrt(num - 1) * sqrt(num + 1));
}

/** Calculate inverse hyperbolic sine for Windows.
 * @param num The double-precision floating point value to calculate
 * @return The inverse hyperbolic sine of the value
 */
static double asinh(double num)
{
	return log(num + sqrt(num * num + 1));
}

/** Calculate inverse hyperbolic tangent for Windows.
 * @param num The double-precision floating point value to calculate
 * @return The inverse hyperbolic tangent of the value
 */
static double atanh(double num)
{
	return 0.5 * log((1 + num) / (1 - num));
}

/** Calculate cube root for Windows.
 * @param num The double-percision floating point value to calculate
 * @return The cube root of the value
 */
static double cbrt(double num)
{
	return pow(num, 1.0 / 3.0);
}

/** Truncate a double-precision floating point value for Windows.
 * @param num The double-precision floating point value to truncate
 * @return The truncated value
 */
static double trunc(double num)
{
	return static_cast<double>(static_cast<int>(num));
}
#endif
/** Determine if the given character is a number.
 * @param chr Character to check
 * @return true if the character is a number, false otherwise
 */
inline static bool is_number(char chr) { return (chr >= '0' && chr <= '9') || chr == '.'; }
/** Determine if the given character is a multiplication or division operator.
 * @param chr Character to check
 * @return true if the character is a multiplication or division operator, false otherwise
 */
inline static bool is_muldiv(char chr) { return chr == '%' || chr == '/' || chr == '*'; }
/** Determine if the given character is an addition or subtraction operator.
 * @param chr Character to check
 * @return true if the character is an addition or subtraction operator, false otherwise
 */
inline static bool is_plusmin(char chr) { return chr == '+' || chr == '-'; }
/** Determine if the given character is an operator of any sort, except for parenthesis.
 * @param chr Character to check
 * @return true if the character is a non-parenthesis operator, false otherwise
 */
inline static bool is_op_noparen(char chr) { return is_plusmin(chr) || is_muldiv(chr) || chr == '^' || chr == 'd'; }
/** Determine if the given character is an operator of any sort.
 * @param chr Character to check
 * @return true if the character is an operator, false otherwise
 */
inline static bool is_operator(char chr) { return chr == '(' || chr == ')' || is_op_noparen(chr); }
/** Determine if the substring portion of the given string is a function.
 * @param str String to check
 * @param pos Starting position of the substring to check, defaults to 0
 * @return 0 if the string isn't a function, or a number corresponding to the length of the function name
 */
inline static unsigned is_function(const Anope::string &str, unsigned pos = 0)
{
	/* We only need a 5 character substring as that's the largest substring we will be looking at */
	Anope::string func = str.substr(pos, 5);
	/* acosh, asinh, atanh, floor, log10, round, trunc */
	Anope::string func_5 = func.substr(0, 5);
	if (func_5.equals_ci("acosh") || func_5.equals_ci("asinh") || func_5.equals_ci("atanh") || func_5.equals_ci("floor") || func_5.equals_ci("log10") || func_5.equals_ci("round") || func_5.equals_ci("trunc"))
		return 5;
	/* acos, asin, atan, cbrt, ceil, cosh, rand, sinh, sqrt, tanh */
	Anope::string func_4 = func.substr(0, 4);
	if (func_4.equals_ci("acos") || func_4.equals_ci("asin") || 
	    func_4.equals_ci("atan") || func_4.equals_ci("cbrt") || 
	    func_4.equals_ci("ceil") || func_4.equals_ci("cosh") || 
	    func_4.equals_ci("rand") || func_4.equals_ci("sinh") || 
	    func_4.equals_ci("sqrt") || func_4.equals_ci("tanh"))
		return 4;
	/* abs, cos, deg, exp, fac, log, max, min, rad, sin, tan */
	Anope::string func_3 = func.substr(0, 3);
	if (func_3.equals_ci("abs") || func_3.equals_ci("cos") || func_3.equals_ci("deg") || 
	    func_3.equals_ci("exp") || func_3.equals_ci("fac") || func_3.equals_ci("log") || 
	    func_3.equals_ci("max") || func_3.equals_ci("min") || func_3.equals_ci("rad") || 
	    func_3.equals_ci("sin") || func_3.equals_ci("tan"))
		return 3;
	/* None of the above */
	return 0;
}
/** Determine the number of arguments that the given function needs.
 * @param str Function string to check
 * @return Returns 1 except for the min, max, and rand functions which return 2.
 */
inline static unsigned function_argument_count(const Anope::string &str)
{
	if (str.equals_ci("max") || str.equals_ci("min") || str.equals_ci("rand"))
		return 2;
	return 1;
}
/** Determine if the substring portion of the given string is a constant (currently only e and pi).
 * @param str String to check
 * @param pos Starting position of the substring to check, defaults to 0
 * @return 0 if the string isn't a constant, or a number corresponding to the length of the constant's name
 */
inline static unsigned is_constant(const Anope::string &str, unsigned pos = 0)
{
	/* We only need a 2 character substring as that's the largest substring we will be looking at */
	Anope::string constant = str.substr(pos, 2);
	/* pi */
	if (constant.substr(0, 2).equals_ci("pi"))
		return 2;
	/* e */
	if (constant.substr(0, 1).equals_ci("e"))
		return 1;
	/* None of the above */
		return 0;
}
/** Determine if the given operator has a higher precendence than the operator on the top of the stack during infix to postfix conversion.
 * @param adding The operator we are adding to the stack, or an empty string if nothing is being added and we just want to remove
 * @param topstack The operator that was at the top of the operator stack
 * @return 0 if the given operator has the same or lower precendence (and won't cause a pop), 1 if the operator has higher precendence (and will cause a pop)
 *
 * In addition to the above in regards to the return value, there are other situations. If the top of the stack is an open parenthesis,
 * or is empty, a 0 will be returned to stop the stack from popping anything else. If nothing is being added to the stack and the previous
 * sitation hasn't occurred, a 1 will be returned to signify to continue popping the stack until the previous sitation occurs. If the operator
 * being added is a function, we return 0 to signify we aren't popping. If the top of the stack is a function, we return 1 to signify we are
 * popping. A -1 is only returned if an invalid operator is given, hopefully that will never happen.
 */
inline static int would_pop(const Anope::string &adding, const Anope::string &topstack)
{
	if (adding.empty())
		return topstack.empty() || topstack == "(" ? 0 : 1;
	if (is_function(adding))
		return 0;
	if (topstack.empty() || topstack == "(")
		return 0;
	if (is_function(topstack))
		return 1;
	if (topstack == adding && adding != "^")
		return 1;
	switch (adding[0])
	{
		case 'd':
			return 0;
		case '^':
			return topstack == "^" || topstack.equals_ci("d") ? 1 : 0;
		case '%':
		case '/':
		case '*':
			return topstack == "^" || topstack.equals_ci("d") || is_muldiv(topstack[0]) ? 1 : 0;
		case '+':
		case '-':
			return is_op_noparen(topstack[0]) ? 1 : 0;
	}
	return -1;
}
/** Convert a double-precision floating point value to a string.
 * @param num The value to convert
 * @return A string containing the value
 */
static Anope::string dtoa(double num)
{
	char temp[33];
	snprintf(temp, sizeof(temp), "%.*g", static_cast<int>(sizeof(temp) - 7), num);
	return temp;
}

/** Round a value to the given number of decimals, originally needed for Windows but also used for other OSes as well due to undefined references.
 * @param val The value to round
 * @param decimals The number of digits after the decimal point, defaults to 0
 * @return The rounded value
 *
 * NOTE: Function is a slightly modified form of the code from this page:
 * http://social.msdn.microsoft.com/forums/en-US/vclanguage/thread/a7d4bf31-6c32-4b25-bc76-21b29f5287a1/
 */
static double my_round(double val, unsigned decimals = 0)
{
	if (!val) /* val must be different from zero to avoid division by zero! */
		return 0;
	double sign = fabs(val) / val; /* we obtain the sign to calculate positive always */
	double tempval = fabs(val * pow(10.0, static_cast<double>(decimals))); /* shift decimal places */
	unsigned tempint = static_cast<unsigned>(tempval);
	double decimalpart = tempval - tempint; /* obtain just the decimal part */
	if (decimalpart >= 0.5)
		tempval = ceil(tempval); /* next integer number if greater or equal to 0.5 */
	else
		tempval = floor(tempval); /* otherwise stay in the current interger part */
	return (tempval * pow(10.0, static_cast<double>(-static_cast<int>(decimals)))) * sign; /* shift again to the normal decimal places */
}

/** Structure to store the infix notation string as well as the positions each character is compared to the original input */
struct Infix
{
	Anope::string str;
	std::vector<unsigned> positions;
	Infix(const Anope::string &newStr, std::vector<unsigned> newPositions)
	{
		this->str = newStr;
		this->positions = newPositions;
	}
	Infix(const Anope::string &newStr, unsigned newPositions[], unsigned num)
	{
		this->str = newStr;
		this->positions = std::vector<unsigned>(newPositions, newPositions + sizeof(unsigned) * num);
	}
};
/** Fix an infix notation equation.
 * @param infix The original infix notation equation
 * @return A fixed infix notation equation
 *
 * This will convert a single % to 1d100, place a 1 in front of any d's that have no numbers before them, change all %'s after a d into 100,
 * add *'s for implicit multiplication, and convert unary -'s to _ for easier parsing later.
 */
static Infix fix_infix(const Anope::string &infix)
{
	if (infix == "%")
	{
		unsigned tmp[] = {0, 0, 0, 0, 0};
		Infix newinfix("1d100", tmp, 5);
		return newinfix;
	}
	bool prev_was_func = false, prev_was_const = false;
	Anope::string newinfix;
	std::vector<unsigned> positions;
	unsigned len = infix.length();
	for (unsigned x = 0; x < len; ++x)
	{
		/* Check for a function, and skip it if it exists */
		unsigned func = is_function(infix, x);
		if (func)
		{
			if (x && is_number(infix[x - 1]))
			{
				newinfix += '*';
				positions.push_back(x);
			}
			newinfix += infix.substr(x, func);
			for (unsigned y = 0; y < func; ++y)
				positions.push_back(x + y);
			x += func - 1;
			prev_was_func = true;
			continue;
		}
		/* Check for a constant, and skip it if it exists */
		unsigned constant = is_constant(infix, x);
		if (constant)
		{
			if (x && is_number(infix[x - 1]))
			{
				newinfix += '*';
				positions.push_back(x);
			}
			newinfix += infix.substr(x, constant);
			for (unsigned y = 0; y < constant; ++y)
				positions.push_back(x + y);
			if (x + constant < len && (is_number(infix[x + constant]) || is_constant(infix, x + constant) || is_function(infix, x + constant)))
			{
				newinfix += '*';
				positions.push_back(x + constant);
			}
			x += constant - 1;
			prev_was_const = true;
			continue;
		}
		char curr = std::tolower(infix[x]);
		if (curr == 'd')
		{
			positions.push_back(x);
			if (!x)
			{
				newinfix += "1d";
				positions.push_back(x);
			}
			else
			{
				if (!is_number(infix[x - 1]) && infix[x - 1] != ')' && !prev_was_const)
				{
					newinfix += '1';
					positions.push_back(x);
				}
				newinfix += 'd';
			}
			if (x != len - 1 && infix[x + 1] == '%')
			{
				newinfix += "100";
				++x;
				positions.push_back(x);
				positions.push_back(x);
			}
		}
		else if (curr == '(')
		{
			if (x && !prev_was_func && (is_number(infix[x - 1]) || prev_was_const))
			{
				newinfix += '*';
				positions.push_back(x);
			}
			newinfix += '(';
			positions.push_back(x);
		}
		else if (curr == ')')
		{
			newinfix += ')';
			positions.push_back(x);
			if (x != len - 1 && (is_number(infix[x + 1]) || infix[x + 1] == '(' || is_constant(infix, x + 1)))
			{
				newinfix += '*';
				positions.push_back(x);
			}
		}
		else if (curr == '-')
		{
			positions.push_back(x);
			if (x != len - 1 && (!x ? 1 : is_op_noparen(std::tolower(infix[x - 1])) || infix[x - 1] == '(' || infix[x - 1] == ','))
			{
				if (infix[x + 1] == '(' || is_function(infix, x + 1))
				{
					newinfix += "0-";
					positions.push_back(x);
				}
				else if (is_number(infix[x + 1]) || is_constant(infix, x + 1))
					newinfix += '_';
				else
					newinfix += '-';
			}
			else
				newinfix += '-';
		}
		else
		{
			newinfix += curr;
			positions.push_back(x);
		}
		prev_was_func = prev_was_const = false;
	}
	positions.push_back(len);
	return Infix(newinfix, positions);
}
/** Validate an infix notation equation.
 * @param infix The infix notation equation to validate
 * @return false for an invalid equation, true for a valid one
 *
 * The validation is as follows:
 * - All functions must have an open parenthesis after them.
 * - A comma must be prefixed by a number or close parenthesis and must be suffixed by a number, open parenthesis, _ for unary minus, constant, or function.
 * - All non-parentheis operators must be prefixed by a number or close parenthesis and suffixed by a number, open parenthesis, _ for unary minus, constant, or function.
 * - All open parentheses must be prefixed by an operator, open parenthesis, or comma and suffixed by a number, an open parenthesis, _ for unary minus, constant, or function.
 * - All close parentheses must be prefixed by a number or close parenthesis and suffixed by an operator, close parenthesis, or comma.
 */
static bool check_infix(const Infix &infix)
{
	bool prev_was_func = false, prev_was_const = false;
	for (unsigned x = 0, len = infix.str.length(); x < len; ++x)
	{
		unsigned position = infix.positions[x];
		/* Check for a function, and skip it if it exists */
		unsigned func = is_function(infix.str, x);
		if (func)
		{
			if ((x + func < len && infix.str[x + func] != '(') || x + func >= len)
			{
				DiceErrPos = infix.positions[x + func >= len ? len : x + func];
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No open parenthesis found after function.");
				return false;
			}
			x += func - 1;
			prev_was_func = true;
			continue;
		}
		/* Check for a constant, and skip it if it exists */
		unsigned constant = is_constant(infix.str, x);
		if (constant)
		{
			x += constant - 1;
			prev_was_const = true;
			continue;
		}
		if (infix.str[x] == ',')
		{
			if (!x ? 1 : !is_number(infix.str[x - 1]) && infix.str[x - 1] != ')' && !prev_was_const)
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number or close parenthesis before comma.");
				return false;
			}
			if (x == len - 1 ? 1 : !is_number(infix.str[x + 1]) && infix.str[x + 1] != '(' && infix.str[x + 1] != '_' && !is_constant(infix.str, x + 1) && !is_function(infix.str, x + 1))
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number or open parenthesis after comma.");
				return false;
			}
		}
		else if (is_op_noparen(infix.str[x]))
		{
			if (!x ? 1 : !is_number(infix.str[x - 1]) && infix.str[x - 1] != ')' && !prev_was_const)
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number or close parenthesis before operator.");
				return false;
			}
			if (x == len - 1 ? 1 : !is_number(infix.str[x + 1]) && infix.str[x + 1] != '(' && infix.str[x + 1] != '_' && !is_constant(infix.str, x + 1) && !is_function(infix.str, x + 1))
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number or open parenthesis after operator.");
				return false;
			}
		}
		else if (infix.str[x] == '(')
		{
			if (x && !is_op_noparen(infix.str[x - 1]) && infix.str[x - 1] != '(' && infix.str[x - 1] != ',' && !prev_was_func)
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No operator or open parenthesis found before current open\nparenthesis.");
				return false;
			}
			if (x != len - 1 && !is_number(infix.str[x + 1]) && infix.str[x + 1] != '(' && infix.str[x + 1] != '_' && !is_constant(infix.str, x + 1) && !is_function(infix.str, x + 1))
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number after current open parenthesis.");
				return false;
			}
		}
		else if (infix.str[x] == ')')
		{
			if (x && !is_number(infix.str[x - 1]) && infix.str[x - 1] != ')' && !prev_was_const)
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No number found before current close parenthesis.");
				return false;
			}
			if (x != len - 1 && !is_op_noparen(infix.str[x + 1]) && infix.str[x + 1] != ')' && infix.str[x + 1] != ',')
			{
				DiceErrPos = position;
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No operator or close parenthesis found after current close\nparenthesis.");
				return false;
			}
		}
		else if (!is_number(infix.str[x]) && !is_muldiv(infix.str[x]) && !is_plusmin(infix.str[x]) && !is_operator(infix.str[x]) && infix.str[x] != '_')
		{
			DiceErrPos = position;
			DiceErrCode = DICE_ERR_PARSE;
			DiceErrStr = _("An invalid character was encountered.");
			return false;
		}
		prev_was_func = prev_was_const = false;
	}
	return true;
}


/** Tokenize an infix notation equation by adding spaces between operators.
 * @param infix The original infix notation equation to tokenize
 * @return A new infix notation equation with spaces between operators
 */
static Infix tokenize_infix(const Infix &infix)
{
	Anope::string newinfix;
	std::vector<unsigned> positions;
	for (unsigned x = 0, len = infix.str.length(); x < len; ++x)
	{
		unsigned position = infix.positions[x], func = is_function(infix.str, x), constant = is_constant(infix.str, x);
		char curr = infix.str[x];
		if (func)
		{
			if (x && !newinfix.empty() && newinfix[newinfix.length() - 1] != ' ')
			{
				newinfix += ' ';
				positions.push_back(position);
			}
			newinfix += infix.str.substr(x, func);
			for (unsigned y = 0; y < func; ++y)
				positions.push_back(infix.positions[x + y]);
			if (x != len - 1)
			{
				newinfix += ' ';
				positions.push_back(infix.positions[x + func]);
			}
			x += func - 1;
		}
		else if (constant)
		{
			if (x && !newinfix.empty() && newinfix[newinfix.length() - 1] != ' ' && newinfix[newinfix.length() - 1] != '_')
			{
				newinfix += ' ';
				positions.push_back(position);
			}
			newinfix += infix.str.substr(x, constant);
			for (unsigned y = 0; y < constant; ++y)
				positions.push_back(infix.positions[x + y]);
			if (x != len - 1)
			{
				newinfix += ' ';
				positions.push_back(infix.positions[x + constant]);
			}
			x += constant - 1;
		}
		else if (curr == ',')
		{
			if (x && !newinfix.empty() && newinfix[newinfix.length() - 1] != ' ')
			{
				newinfix += ' ';
				positions.push_back(position);
			}
			newinfix += ',';
			positions.push_back(position);
			if (x != len - 1)
			{
				newinfix += ' ';
				positions.push_back(position);
			}
		}
		else if (is_operator(curr))
		{
			if (x && !newinfix.empty() && newinfix[newinfix.length() - 1] != ' ')
			{
				newinfix += ' ';
				positions.push_back(position);
			}
			newinfix += curr;
			positions.push_back(position);
			if (x != len - 1)
			{
				newinfix += ' ';
				positions.push_back(position);
			}
		}
		else
		{
			newinfix += curr;
			positions.push_back(position);
		}
	}
	return Infix(newinfix, positions);
}


/** Base class for values in a postfix equation */
class PostfixValueBase
{
	 public:
		PostfixValueBase() { }
		virtual ~PostfixValueBase() { }
		virtual void Clear() = 0;
};

/** Version of PostfixValue for double */
class PostfixValueDouble : public PostfixValueBase
{
	double *val;
	public:
		PostfixValueDouble() : PostfixValueBase(), val(NULL) { }
		PostfixValueDouble(double *Val) : PostfixValueBase(), val(Val) { }
		PostfixValueDouble(const PostfixValueDouble &Val) : PostfixValueBase(), val(Val.val) { }
		PostfixValueDouble &operator=(const PostfixValueDouble &Val)
		{
			if (this != &Val)
				*val = *Val.val;
			return *this;
		}
		const double *Get() const
		{
			return val;
		}
		void Clear()
		{
			delete val;
		}
};

/** Version of PostfixValue for Anope::string */
class PostfixValueString : public PostfixValueBase
{
	Anope::string *val;
	public:
		PostfixValueString() : PostfixValueBase(), val(NULL) { }
		PostfixValueString(Anope::string *Val) : PostfixValueBase(), val(Val) { }
		PostfixValueString(const PostfixValueString &Val) : PostfixValueBase(), val(Val.val) { }
		PostfixValueString &operator=(const PostfixValueString &Val)
		{
			if (this != &Val)
				*val = *Val.val;
			return *this;
		}
		const Anope::string *Get() const
		{
			return val;
		}
		void Clear()
		{
			delete val;
		}
};

/** Enumeration for PostfixValue to determine it's type */
enum PostfixValueType
{
	POSTFIX_VALUE_DOUBLE,
	POSTFIX_VALUE_STRING
};

typedef std::vector<std::pair<PostfixValueBase *, PostfixValueType> > Postfix;

/** Clears the memory of the given Postfix vector.
 * @param postfix The Postfix vector to clean up
 */
static void cleanup_postfix(Postfix &postfix)
{
	for (unsigned y = 0, len = postfix.size(); y < len; ++y)
	{
		postfix[y].first->Clear();
		delete postfix[y].first;
	}
	postfix.clear();
}
/** Adds a double value to the Postfix vector.
 * @param postfix The Postfix vector to add to
 * @param dbl The double value we are adding
 */
static void add_to_postfix(Postfix &postfix, double dbl)
{
	double *dbl_ptr = new double(dbl);
	postfix.push_back(std::make_pair(new PostfixValueDouble(dbl_ptr), POSTFIX_VALUE_DOUBLE));
}
/** Adds an Anope::string value to the Postfix vector.
 *  * @param postfix The Postfix vector to add to
 *   * @param str The Anope::string value we are adding
 *    */
static void add_to_postfix(Postfix &postfix, const Anope::string &str)
{
	Anope::string *str_ptr = new Anope::string(str);
	postfix.push_back(std::make_pair(new PostfixValueString(str_ptr), POSTFIX_VALUE_STRING));
}
/** Convert an infix notation equation to a postfix notation equation, using the shunting-yard algorithm.
 * @param infix The infix notation equation to convert
 * @return A postfix notation equation
 *
 * Numbers are always stored in the postfix notation equation immediately, and operators are kept on a stack until they are
 * needed to be added to the postfix notation equation.
 * The conversion process goes as follows:
 * - Firstly, verify that the end of the input is a number, ignoring close parentheses.
 * - Iterate through the infix notation equation, doing the following on each operation:
 *   - When a _ is encountered, add the number following it to the postfix notation equation, but make sure it's negative.
 *   - When a number is encountered, add it to the postfix notation equation.
 *   - When a function is encountered, add it to the operator stack.
 *   - When a constant is encountered, convert it to a number and add it to the postfix notation equation.
 *   - When an operator is encountered:
 *     - Check if we had any numbers prior to the operator, and fail if there were none.
 *     - Always add open parentheses to the operator stack.
 *     - When a close parenthesis is encountered, pop all operators until we get to an open parenthesis or the stack 
 *       becomes empty, failing on the latter.
 *     - For all other operators, pop the stack if needed then add the operator to the stack.
 *   - When a comma is encountered, do the same as above for when a close parenthesis is encountered, but also check 
 *     to make sure there was a function prior to the open parenthesis (if there is one).
 *   - If anything else is encountered, fail as it is an invalid value.
 * - If there were operators left on the operator stack, pop all of them, failing if anything is left on the stack 
 *   (an open parenthesis will cause this).
 */
static Postfix infix_to_postfix(const Infix &infix)
{
	Postfix postfix;
	unsigned len = infix.str.length(), x = 0;
	bool prev_was_close = false, prev_was_number = false;
	std::stack<Anope::string> op_stack;
	spacesepstream tokens(infix.str);
	Anope::string token, lastone;
	while (tokens.GetToken(token))
	{
		if (token[0] == '_')
		{
			double number = 0.0;
			if (is_constant(token, 1))
			{
				if (token.substr(1).equals_ci("e"))
					number = -exp(1.0);
				else if (token.substr(1).equals_ci("pi"))
					number = -atan(1.0) * 4;
			}
			else
				number = -atof(token.substr(1).c_str());
			if (is_infinite(number) || is_notanumber(number))
			{
				DiceErrCode = is_infinite(number) ? DICE_ERR_OVERUNDERFLOW : DICE_ERR_UNDEFINED;
				cleanup_postfix(postfix);
				return postfix;
			}
			add_to_postfix(postfix, number);
			prev_was_number = true;
		}
		else if (is_number(token[0]))
		{
			double number = atof(token.c_str());
			if (is_infinite(number) || is_notanumber(number))
			{
				DiceErrCode = is_infinite(number) ? DICE_ERR_OVERUNDERFLOW : DICE_ERR_UNDEFINED;
				cleanup_postfix(postfix);
				return postfix;
			}
			add_to_postfix(postfix, number);
			prev_was_number = true;
		}
		else if (is_function(token))
			op_stack.push(token);
		else if (is_constant(token))
		{
			double number = 0.0;
			if (token.equals_ci("e"))
				number = exp(1.0);
			else if (token.equals_ci("pi"))
				number = atan(1.0) * 4;
			add_to_postfix(postfix, number);
			prev_was_number = true;
		}
		else if (is_operator(token[0]))
		{
			lastone = op_stack.empty() ? "" : op_stack.top();
			if (!prev_was_number && token != "(" && token != ")" && !prev_was_close)
			{
				DiceErrPos = infix.positions[x];
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("No numbers were found before the operator was encountered.");
				cleanup_postfix(postfix);
				return postfix;
			}
			prev_was_number = false;
			if (token == "(")
			{
				op_stack.push(token);
				prev_was_close = false;
			}
			else if (token == ")")
			{
				while (would_pop(token, lastone))
				{
					add_to_postfix(postfix, lastone);
					op_stack.pop();
					lastone = op_stack.empty() ? "" : op_stack.top();
				}
				if (lastone != "(")
				{
					DiceErrPos = infix.positions[x];
					DiceErrCode = DICE_ERR_PARSE;
					DiceErrStr = _("A close parenthesis was found but not enough open\nparentheses were found before it.");
					cleanup_postfix(postfix);
					return postfix;
				}
				else
					op_stack.pop();
				prev_was_close = true;
			}
			else
			{
				if (!would_pop(token, lastone))
					op_stack.push(token);
				else
				{
					while (would_pop(token, lastone))
					{
						add_to_postfix(postfix, lastone);
						op_stack.pop();
						lastone = op_stack.empty() ? "" : op_stack.top();
					}
					op_stack.push(token);
				}
				prev_was_close = false;
			}
		}
		else if (token[0] == ',')
		{
			lastone = op_stack.empty() ? "" : op_stack.top();
			while (would_pop(token, lastone))
			{
				add_to_postfix(postfix, lastone);
				op_stack.pop();
				lastone = op_stack.empty() ? "" : op_stack.top();
			}
			if (lastone != "(")
			{
				DiceErrPos = infix.positions[x];
				DiceErrCode = DICE_ERR_PARSE;
				DiceErrStr = _("A comma was encountered outside of a function.");
				cleanup_postfix(postfix);
				return postfix;
			}
			else
			{
				Anope::string paren = lastone;
				op_stack.pop();
				lastone = op_stack.empty() ? "" : op_stack.top();
				if (!is_function(lastone))
				{
					DiceErrPos = infix.positions[x];
					DiceErrCode = DICE_ERR_PARSE;
					DiceErrStr = _("A comma was encountered outside of a function.");
					cleanup_postfix(postfix);
					return postfix;
				}
				else
					op_stack.push(paren);
			}
		}
		else
		{
			DiceErrPos = infix.positions[x];
			DiceErrCode = DICE_ERR_PARSE;
			DiceErrStr = _("An invalid character was encountered.");
			cleanup_postfix(postfix);
			return postfix;
		}
		x += token.length() + (x ? 1 : 0);
	}
	if (!op_stack.empty())
	{
		lastone = op_stack.top();
		while (would_pop("", lastone))
		{
			add_to_postfix(postfix, lastone);
			op_stack.pop();
			if (op_stack.empty())
				break;
			else
				lastone = op_stack.top();
		}
		if (!op_stack.empty())
		{
			DiceErrPos = infix.positions[len];
			DiceErrCode = DICE_ERR_PARSE;
			DiceErrStr = _("There are more open parentheses than close parentheses.");
			cleanup_postfix(postfix);
			return postfix;
		}
	}
	return postfix;
}
/** Evaluate a postfix notation equation.
 * @param The postfix notation equation to evaluate
 * @return The final result after calcuation of the equation
 *
 * The evaluation pops a single value from the operand stack for a function, and 2 values from the 
 * operand stack for an operator. The result of either one is placed back on the operand stack, 
 * hopefully leaving a single result at the end.
 */
double eval_postfix(const Postfix &postfix)
{
	double val = 0.0;
	std::stack<double> num_stack;
	for (unsigned x = 0, len = postfix.size(); x < len; ++x)
	{
		if (postfix[x].second == POSTFIX_VALUE_STRING)
		{
			const PostfixValueString *str = dynamic_cast<const PostfixValueString *>(postfix[x].first);
			const Anope::string *token_ptr = str->Get();
			Anope::string token = token_ptr ? *token_ptr : "";
			if (token.empty())
			{
				DiceErrCode = DICE_ERR_STACK;
				DiceErrStr = _("An empty token was found.");
				return 0;
			}
			if (is_function(token))
			{	
				unsigned function_arguments = function_argument_count(token);
				if (num_stack.empty() || num_stack.size() < function_arguments)
				{
					DiceErrCode = DICE_ERR_STACK;
					DiceErrStr = _("Not enough numbers for function.");
					return 0;
				}
				double val1 = num_stack.top();
				num_stack.pop();
				if (token.equals_ci("abs"))
					val = fabs(val1);
				else if (token.equals_ci("acos"))
				{
					if (fabs(val1) > 1)
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = acos(val1);
				}
				else if (token.equals_ci("acosh"))
				{
					if (val1 < 1)
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = acosh(val1);
				}
				else if (token.equals_ci("asin"))
				{
					if (fabs(val1) > 1)
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = asin(val1);
				}
				else if (token.equals_ci("asinh"))
					val = asinh(val1);
				else if (token.equals_ci("atan"))
					val = atan(val1);
				else if (token.equals_ci("atanh"))
				{
					if (fabs(val1) >= 1)
					{
						DiceErrCode = fabs(val1) == 1 ? DICE_ERR_DIV0 : DICE_ERR_UNDEFINED;
						return 0;
					}
					val = atanh(val1);
				}
				else if (token.equals_ci("cbrt"))
					val = cbrt(val1);
				else if (token.equals_ci("ceil"))
					val = ceil(val1);
				else if (token.equals_ci("cos"))
					val = cos(val1);
				else if (token.equals_ci("cosh"))
					val = cosh(val1);
				else if (token.equals_ci("deg"))
					val = val1 * 45.0 / atan(1.0);
				else if (token.equals_ci("exp"))
					val = exp(val1);
				else if (token.equals_ci("fac"))
				{
					if (static_cast<int>(val1) < 0)
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = 1;
					for (unsigned n = 2; n <= static_cast<unsigned>(val1); ++n)
						val *= n;
				}
				else if (token.equals_ci("floor"))
					val = floor(val1);
				else if (token.equals_ci("log"))
				{
					if (val1 <= 0)
					{
						DiceErrCode = DICE_ERR_DIV0;
						return 0;
					}
					val = log(val1);
				}
				else if (token.equals_ci("log10"))
				{
					if (val1 <= 0)
					{
						DiceErrCode = DICE_ERR_DIV0;
						return 0;
					}
					val = log10(val1);
				}
				else if (token.equals_ci("max"))
				{
					double val2 = val1;
					val1 = num_stack.top();
					num_stack.pop();
					val = std::max(val1, val2);
				}
				else if (token.equals_ci("min"))
				{
					double val2 = val1;
					val1 = num_stack.top();
					num_stack.pop();
					val = std::min(val1, val2);
				}
				else if (token.equals_ci("rad"))
					val = val1 * atan(1.0) / 45.0;
				else if (token.equals_ci("rand"))
				{
					double val2 = val1;
					val1 = num_stack.top();
					num_stack.pop();
					if (val1 > val2)
					{
						double tmp = val2;
						val2 = val1;
						val1 = tmp;
					}
					val = Dice(static_cast<int>(val1), static_cast<int>(val2));
					Anope::string buf = Anope::string("rand(") + stringify(static_cast<int>(val1)) + "," + stringify(static_cast<int>(val2)) + ")=(" + stringify(static_cast<int>(val)) + ") ";
					DiceShortExOutput += buf;
					DiceExOutput += buf;
				}
				else if (token.equals_ci("round"))
					val = my_round(val1);
				else if (token.equals_ci("sin"))
					val = sin(val1);
				else if (token.equals_ci("sinh"))
					val = sinh(val1);
				else if (token.equals_ci("sqrt"))
				{
					if (val1 < 0)
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = sqrt(val1);
				}
				else if (token.equals_ci("tan"))
				{
					if (!fmod(val1 + 2 * atan(1.0), atan(1.0) * 4))
					{
						DiceErrCode = DICE_ERR_UNDEFINED;
						return 0;
					}
					val = tan(val1);
				}
				else if (token.equals_ci("tanh"))
					val = tanh(val1);
				else if (token.equals_ci("trunc"))
					val = trunc(val1);
				if (is_infinite(val) || is_notanumber(val))
				{
					DiceErrCode = is_infinite(val) ? DICE_ERR_OVERUNDERFLOW : DICE_ERR_UNDEFINED;
					return 0;
				}
				num_stack.push(val);
			} //end if(is_function)
			else if (is_operator(token[0]) && token.length() == 1)
			{	
				if (num_stack.empty() || num_stack.size() < 2)
				{	
					DiceErrCode = DICE_ERR_STACK;
					DiceErrStr = _("Not enough numbers for operator.");
					return 0;
				}
				double val2 = num_stack.top();
				num_stack.pop();
				double val1 = num_stack.top();
				num_stack.pop();
				switch (token[0])
				{
					case '+':
						val = val1 + val2;
						break;
					case '-':
						val = val1 - val2;
						break;
					case '*':
						val = val1 * val2;
						break;
					case '/':
						if (!val2)
						{
							DiceErrCode = DICE_ERR_DIV0;
							return 0;
						}
						val = val1 / val2;
						break;
					case '%':
						if (!val2)
						{
							DiceErrCode = DICE_ERR_DIV0;
							return 0;
						}
						val = fmod(val1, val2);
						break;
					case '^':
						if (val1 < 0 && static_cast<double>(static_cast<int>(val2)) != val2)
						{
							DiceErrCode = DICE_ERR_UNDEFINED;
							return 0;
						}
						if (!val1 && !val2)
						{
							DiceErrCode = DICE_ERR_DIV0;
							return 0;
						}
						if (!val1 && val2 < 0)
						{
							DiceErrCode = DICE_ERR_OVERUNDERFLOW;
							return 0;
						}
						val = pow(val1, val2);
						break;
					case 'd':
						if (val1 < 1 || val1 > DICE_MAX_DICE)
						{
							DiceErrCode = DICE_ERR_UNACCEPT_DICE;
							DiceErrNum = static_cast<int>(val1);
							return 0;
						}
						if (val2 < 1 || val2 > DICE_MAX_SIDES)
						{
							DiceErrCode = DICE_ERR_UNACCEPT_SIDES;
							DiceErrNum = static_cast<int>(val2);
							return 0;
						}
						int ival1 = (int)(val1);
						unsigned uval2 = (unsigned)(val2);
						int ival = Dice(ival1, uval2);
						val = (double)(ival);
						break;
				}
				if (is_infinite(val) || is_notanumber(val))
				{
					DiceErrCode = is_infinite(val) ? DICE_ERR_OVERUNDERFLOW : DICE_ERR_UNDEFINED;
					return 0;
				}
				num_stack.push(val);
			}
		}
		else
		{
			const PostfixValueDouble *dbl = dynamic_cast<const PostfixValueDouble *>(postfix[x].first);
			const double *val_ptr = dbl->Get();
			if (!val_ptr)
			{
				DiceErrCode = DICE_ERR_STACK;
				DiceErrStr = _("An empty number was found.");
				return 0;
			}
			num_stack.push(*val_ptr);
		}
	}
	val = num_stack.top();
	num_stack.pop();
	if (!num_stack.empty())
	{
		DiceErrCode = DICE_ERR_STACK;
		DiceErrStr = _("Too many numbers were found as input.");
		return 0;
	}
	return val;
}

/** Parse an infix notation expression and convert the expression to postfix notation.
 * @param infix The original expression, in infix notation, to convert to postfix notation
 * @return A postfix notation expression equivilent to the infix notation expression given, or an empty string if the infix notation expression could not be parsed or converted
 */
Postfix DoParse(const Anope::string &infix)
{
	Infix infixcpy = fix_infix(infix);
	Postfix postfix;
	if (infixcpy.str.empty())
		return postfix;
	if (!check_infix(infixcpy))
		return postfix;
	Infix tokenized_infix = tokenize_infix(infixcpy);
	if (tokenized_infix.str.empty())
		return postfix;
	postfix = infix_to_postfix(tokenized_infix);
	return postfix;
}

/** Evaluate a postfix notation expression.
 * @param postfix The postfix notation expression to evaluate
 * @return The final result after evaluation
 */
double DoEvaluate(const Postfix &postfix)
{
	double ret = 0.0;
	DiceErrCode = DICE_ERR_NONE;
	DiceErrPos = 0;
	DiceExOutput.clear();
	DiceShortExOutput.clear();
	ret = eval_postfix(postfix);
	if (ret > INT_MAX || ret < INT_MIN)
		DiceErrCode = DICE_ERR_OVERUNDERFLOW;
	return ret;
}

/** DiceServ's error handler, if any step along the way fails, this will display the cause of the error to the user.
 * @param u The user that invoked DiceServ
 * @param dice The dice expression that was used to invoke DiceServ
 * @param ErrorPos The position of the error (usually the same DiceErrPos, but not always)
 * @return true in almost every case to signify there was an error, false if an unknown error code was passed in or no error code was set
 */
bool ErrorHandler(CommandSource &source, const Anope::string &dice, unsigned ErrorPos)
{
	bool WasError = true;
	switch (DiceErrCode)
	{
		case DICE_ERR_PARSE:
		{
			source.Reply(_("During parsing, an error was found in the following\nexpression:"));
			source.Reply(" %s", dice.c_str());
			Anope::string spaces(ErrorPos > dice.length() ? dice.length() : ErrorPos+1, '-');
			source.Reply(">%s^", spaces.c_str());
			source.Reply(_("Error description is as follows:"));
			source.Reply("%s", DiceErrStr.c_str());
			break;
		}
		case DICE_ERR_DIV0:
			source.Reply(_("Division by 0 in following expression:"));
			source.Reply(" %s", dice.c_str());
			break;
		case DICE_ERR_UNDEFINED:
			source.Reply(_("Undefined result in following expression:"));
			source.Reply(" %s", dice.c_str());
			break;
		case DICE_ERR_UNACCEPT_DICE:
			if (DiceErrNum <= 0)
				source.Reply(_("The number of dice that you entered (%d) was under\n1. Please enter a number between 1 and %d."), DiceErrNum, DICE_MAX_DICE);
			else
				source.Reply(_("The number of dice that you entered (%d) was over the\nlimit of %d. Please enter a lower number of dice."), DiceErrNum, DICE_MAX_DICE);
			break;
		case DICE_ERR_UNACCEPT_SIDES:
			if (DiceErrNum <= 0)
				source.Reply(_("The number of sides that you entered (%d) was under\n1. Please enter a number between 1 and %d."), DiceErrNum, DICE_MAX_SIDES);
			else
				source.Reply(_("The number of sides that you entered (%d) was over the\nlimit of %d. Please enter a lower number of sides."), DiceErrNum, DICE_MAX_SIDES);
			break;
		case DICE_ERR_OVERUNDERFLOW:
			source.Reply(_("Dice results in following expression resulted in either\noverflow or underflow:"));
			source.Reply(" %s", dice.c_str());
			break;
		case DICE_ERR_STACK:
			source.Reply(_("The following roll expression could not be properly\nevaluated, please try again or let an administrator know."));
			source.Reply(" %s", dice.c_str());
			source.Reply(_("Error description is as follows:"));
			source.Reply("%s", DiceErrStr.c_str());
			break;
		default:
			WasError = false;
	}
	return WasError;
}



/*
 * Handle RPGSERVs ROLL command
 */

class CommandRSRoll : public Command
{
 public:
	CommandRSRoll(Module *creator) : Command(creator, "rpgserv/roll", 1, 3)
	{
		this->SetDesc(_("Calculates the resulting dice value, even when using advanced math functions.")) ;
		this->SetSyntax(_("\002\037<dice equation>\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		double dresult=0.0;
		int X = 0;
		int Y = 0;
		const Anope::string &dice = params[0];
		const Anope::string &chan = params.size() > 1 ? params[1] : "";
		const Anope::string &message = params.size() > 2 ? params[2] : "";
		Anope::string target = "";
		Anope::string mydice = "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
		DiceErrCode=DICE_ERR_NONE;
		DiceErrNum=0;
		DiceErrPos=0;
		Postfix result;

		if(!chan.empty()) 
		{
			if (chan.c_str()[0] == '#')
			{
				if (!IRCD->IsChannelValid(chan))
					source.Reply(CHAN_X_INVALID, chan.c_str());
				else
				{
					ChannelInfo *ci = ChannelInfo::Find(chan);
					if (!ci && u)
						target = chan.c_str();
//						source.Reply(CHAN_X_NOT_IN_USE, chan.c_str());
					else
					{
						target = ci->name.c_str(); 
					}
				}
			}
			else
			{
				u2 = User::Find(chan, true);
				if (!u2) 
				{
					source.Reply(NICK_X_NOT_IN_USE, chan.c_str());
					return;
				}
				else 
				{
					target = u2->nick.c_str();
				}
			}
		}
		Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << chan << " " << message;
		if (dice.equals_ci("joint"))
		{
			if (!target.empty()) 
			{
				IRCD->SendAction(bi, target.c_str(), "passes a \'cigarette\' to %s",u->nick.c_str());
				IRCD->SendPrivmsg(bi, target.c_str(), "If you get busted I never saw you!");
			}
			else
			{
				u->SendMessage(bi, _(" Psst! If you get busted I never saw you!"));
				u->SendMessage(bi, _("Here you go %s.  One \'cigarette\'."),u->nick.c_str());
			}
			return;
		}
		else if (dice.equals_ci("eyes"))
		{
			if (!target.empty()) 
			{
				IRCD->SendPrivmsg(bi, target.c_str(), "%s rolls eyes.",u->nick.c_str());
				IRCD->SendPrivmsg(bi, target.c_str(), "+-- 1 1 ... snake eyes!");
			}
			else
			{
				u->SendMessage(bi, _("%s rolls eyes."),u->nick.c_str());
				u->SendMessage(bi, _("+-- 1 1 ... snake eyes!"));
			}
			return;
		}
		else if (dice.equals_ci("%d") || dice.equals_ci("percent") || dice.equals_ci("percentile") || dice.equals_ci("%") || dice.equals_ci("d%")) 
		{
			X = 1;
			Y = 100;
			mydice = stringify(X) + "d" + stringify(Y);
		}
		else if (dice.equals_ci("save") || dice.equals_ci("check") || dice.equals_ci("hit") || dice.equals_ci("attack")) 
		{
			X = 1;
			Y = 20;
			mydice = stringify(X) + "d" + stringify(Y);
		}
		else if (dice.equals_ci("init") || dice.equals_ci("initative")) 
		{
			X = 1;
			Y = 10;
			mydice = stringify(X) + "d" + stringify(Y);
		}
		if(!mydice.empty()) 
		{
			result = DoParse(mydice);
			if(ErrorHandler(source, mydice, DiceErrPos)) 
			{
				return;
			}
		}
		else
		{
			result = DoParse(dice);
			if(ErrorHandler(source, dice, DiceErrPos)) 
			{
				return;
			}
		}
		dresult = DoEvaluate(result);
		if (!target.empty())
		{
			if (!u2)
			{
			   IRCD->SendPrivmsg(bi, target.c_str(), "%s rolled '%s':  %s", u->nick.c_str(), dice.c_str(), message.c_str());
			   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Result: %s ", stringify(dresult).c_str());
         }
         else
         {
			   u2->SendMessage(bi, _("%s rolled '%s':  %s"), u->nick.c_str(), dice.c_str(), message.c_str());
			   u2->SendMessage(bi, _("+-- Result: %s "), stringify(dresult).c_str());
         }
			source.Reply("%s rolled '%s':  %s", u->nick.c_str(), dice.c_str(), message.c_str());
			source.Reply("+-- Result: %s ",  stringify(dresult).c_str());
		}
		else
		{
			u->SendMessage(bi, _("%s rolled '%s': %s"), u->nick.c_str(), dice.c_str(), message.c_str());
			u->SendMessage(bi, _("+-- Result: %s "), stringify(dresult).c_str());
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		if(subcommand.empty()) {
			this->SendSyntax(source);
			source.Reply(" ");
			source.Reply(_("Calculate total, based on equation, including random dice.\n"
				" \n"
				"This command is to simulate the rolling of polyhedral\n"
				"dice. it will handle dice with up to 1000 sides, &\n"
				"quantities of dice up to 1000. \n"
				" \n"
				"The <equation> argument is required and specfies what\n"
				"dice to roll and any additional functions which need\n"
				"to be preformed with said dice. Dice format is in\n"
				"common dice notation, or one of several keywords\n"
				"which serve as alias to that notation. See '/msg rpgserv \n"
				"help roll expressions' for more detaisl on dice notation. \n"
				"See '/msg rpgserv help calc functions' for more details for\n"
			        "the mathmatical functions available.\n"
				" \n"
				"The [optional where] paramater is as implied an\n"
				"optional paramater. It specifies a location in addition\n"
				"to the user to which output is sent. This can be either\n"
				"a channel or another username. No error checking is\n"
				"performed on this parameter and if invalid information\n"
				"is supplied it will simply output to the user alone.\n"
				" \n"
				"The [optional message] paramter is useful to record, \n"
				"publicly, what your intent is or modifications to the \n"
				"roll are.\n"
				" \n"));
			return true;
		}
		else if ( subcommand.equals_ci("expressions") || subcommand.equals_ci("syntax") )
		{
			source.Reply(_("dice expression must be in the form of: xdy\n"
				" \n"
				"x and y can support very complex forms of expressions. In\n"
				"order to get an actual dice roll, you must use something in\n"
				"the format of: [z]dw, where z is the number of dice to\n"
				"be thrown, and w is the number of sides on each die. z is\n"
				"optional, will default to 1 if not given. Please note that\n"
				"the sides or number of dice can not be 0 or negative, and\n"
				"both can not be greater than 9999.\n"
				" \n"
				"To roll what is called a \"percentile\" die, or a 100-sided\n"
				"die, you can use %% as your roll expression, or include d%%\n"
				"within your roll expression. For the former, the expression\n"
				"will be replaced with 1d100, whereas for the latter, the\n"
				"%% in the expression will be replaced with a 100. For all\n"
				"other cases, the %% will signify modulus of the numbers\n"
				"before and after it, the modulus being the remainder that\n"
				"you'd get from dividing the first number by the second\n"
				"number.\n"
				" \n"
				"The following math operators can be used in expressions:\n"
				" \n"
				"+ - * / ^ %% (in addition to 'd' for dice rolls and\n"
				"parentheses to force order of operatons.)\n"
			" \n"));
			return true;
		}
		source.Reply(_("No help for roll %s."),subcommand.c_str());
		return true;
	}
};

/*
 * Handle RPGSERV CALC command
 */
class CommandRSCalc : public Command
{

  public:
	CommandRSCalc(Module *creator) : Command(creator, "rpgserv/calc", 1, 3)
	{
		this->SetDesc(_("Calculates a value, even when using random dice.")) ;
		this->SetSyntax(_("\002\037<equation>\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		double dresult=0.0;
		const Anope::string &dice = params[0];
		const Anope::string &chan = params.size() > 1 ? params[1] : "";
		const Anope::string &message = params.size() > 2 ? params[2] : "";
		Anope::string target = "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
		DiceErrCode=DICE_ERR_NONE;
		DiceErrNum=0;
		DiceErrPos=0;
		if(!chan.empty()) 
		{
			if (chan.c_str()[0] == '#')
			{
				ChannelInfo *ci = ChannelInfo::Find(chan);
				if (!IRCD->IsChannelValid(chan))
					source.Reply(CHAN_X_INVALID, chan.c_str());
				else if (!ci && u)
					source.Reply(CHAN_X_NOT_IN_USE, chan.c_str());
				else
				{
					target = ci->name.c_str(); 
				}
			}
			else
			{
				u2 = User::Find(chan, true);
				if (!u2) 
				{
					source.Reply(NICK_X_NOT_IN_USE, chan.c_str());
					return;
				}
				else 
				{
					target = u2->nick.c_str();
				}
			}
		}
		Postfix result = DoParse(dice);
		if(ErrorHandler(source, dice, DiceErrPos)) 
		{
			return;
		}
		dresult = DoEvaluate(result);
		Log(LOG_COMMAND,source, this) << "to generate dice of " << dice.c_str() << " with a message of:" << message.c_str() << " to " << target.c_str();
		if (!target.empty())
		{
			if (!u2)
			{
			   IRCD->SendPrivmsg(bi, target.c_str(), "%s requested calculation of '%s':  %s", u->nick.c_str(), dice.c_str(), message.c_str());
			   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Result: %s ", stringify(dresult).c_str());
         }
         else
         {
			   u2->SendMessage(bi, _("%s requested calculation of '%s':  %s"), u->nick.c_str(), dice.c_str(), message.c_str());
			   u2->SendMessage(bi, _("+-- Result: %s "), stringify(dresult).c_str());
         }
			source.Reply("%s requested calculation of '%s':  %s", u->nick.c_str(), dice.c_str(), message.c_str());
			source.Reply("+-- Result: %s ",  stringify(dresult).c_str());
		}
		else
		{
			u->SendMessage(bi, _("%s requested calculation of '%s': %s"), u->nick.c_str(), dice.c_str(), message.c_str());
			u->SendMessage(bi, _("+-- Result: %s "), stringify(dresult).c_str());
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		if(subcommand.empty())
		{
			this->SendSyntax(source);
			source.Reply(" ");
			source.Reply(_("Calculate total, based on equation, including dice.\n"
				" \n"
				"The <equation> argument is required and specfies what\n"
				"dice to roll and any additional functions which need\n"
				"to be preformed with said dice. Dice format is in\n"
				"common dice notation, or one of several keywords\n"
				"which serve as alias to that notation. See '/msg rpgserv \n"
				"help calc functions' for more details. See '/msg rpgserv \n"
				"help roll expressions' for more details on dice syntax.\n"
				" \n"
				"The [optional where] paramater is as implied an\n"
				"optional paramater. It specifies a location in addition\n"
				"to the user to which output is sent. This can be either\n"
				"a channel or another username. No error checking is\n"
				"performed on this parameter and if invalid information\n"
				"is supplied it will simply output to the user alone.\n"
				" \n"
				"The [optional message] paramter is useful to record, \n"
				"publicly, what your intent is or modifications to the \n"
				"roll are.\n"
				" \n"));
			return true;
		}
		else if( subcommand.equals_ci("functions") || subcommand.equals_ci("syntax") ) 
		{
			source.Reply(" ");
			source.Reply(_("Math functions rupported:\n"
				" \n"
				"    abs(x)         Absolute value of x\n"
				"    acos(x)        Arc cosine of x\n"
				"    acosh(x)       Inverse hyperbolic cosine of x\n"
				"    asin(x)        Arc sine of x\n"
				"    asinh(x)       Inverse hyperbolic sine of x\n"
				"    atan(x)        Arc tangent of x\n"
				"    atanh(x)       Inverse hyperbolic tangent of x\n"
				"    cbrt(x)        Cube root of x\n"
				"    ceil(x)        The next smallest integer greater than\n"
				"                   or equal to x\n"
				"    cos(x)         Cosine of x\n"
				"    cosh(x)        Hyperbolic cosine of x\n"
				"    deg(x)         Convert x from radians to degrees\n"
				"    exp(x)         e (exponential value) to the power\n"
				"                   of x\n"
				"    fac(x)         Factorial of x\n"
				"    floor(x)       The next largest integer less than or\n"
				"                   equal to x\n"
				"    log(x)         Natural logarithm of x\n"
				"    log10(x)       Logarithm of x to base 10\n"
				"    max(x,y)       Maximum of x and y\n"
				"    min(x,y)       Minimum of x and y\n"
				"    rad(x)         Convert x from degrees to radians\n"
				"    rand(x,y)      Random value between x and y\n"
				"    round(x)       Round x to the nearest integer\n"
				"    sin(x)         Sine of x\n"
				"    sinh(x)        Hyperbolic sine of x\n"
				"    sqrt(x)        Square root of x\n"
				"    tan(x)         Tangent of x\n"
				"    tanh(x)        Hyperbolic tangent of x\n"
				"    trunc(x)       The integral portion of x\n"
				" \n"
				"NOTE: All trigonometric functions above (sine, cosine and\n"
				"tangent) return their values in radians.\n"
				" \n"
				"The following math operators can be used in expressions:\n"
				" \n"
				"+ - * / ^ %% (in addition to 'd' for dice rolls and\n"
				"parentheses to force order of operatons.)\n"
				" \n"
				"The following math constants are\n"
				"also recognized and will be filled in automatically:\n"
				" \n"
				"    e              Exponential growth constant\n"
				"                   2.7182818284590452354\n"
				"    pi             Archimedes' constant\n"
				"                   3.14159265358979323846\n"
				" \n"
				"The dice roller will also recognize if you want to have a\n"
				"negative number when prefixed with a -. This will not cause\n"
				"problems even though it is also used for subtraction."
				" \n"
			));
			return true;
		}
		source.Reply(_("No help for calc %s"),subcommand.c_str());
		return true;
	}
};

class RpgServCore : public Module
{
	Reference<BotInfo> RpgServ;
	std::vector<Anope::string> defaults;
	bool always_lower;
	CommandRSCalc commandrscalc;
	CommandRSRoll commandrsroll;

 public:
	RpgServCore(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, PSEUDOCLIENT | EXTRA),
		always_lower(false), commandrscalc(this), commandrsroll(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}

        void OnReload(Configuration::Conf *conf) anope_override
	{
		const Anope::string &rsnick = conf->GetModule(this)->Get<const Anope::string>("client");
		if(rsnick.empty()) 
			throw ConfigException(Module::name + ": <client> must be defined");

		BotInfo *bi = BotInfo::Find(rsnick, true);
		if (!bi)
			throw ConfigException(Module::name + ": no bot named " + rsnick);

		RpgServ = bi;
	}

        EventReturn OnBotPrivmsg(User *u, BotInfo *bi, Anope::string &message) anope_override
	{
		if (bi == RpgServ && Config->GetModule(this)->Get<bool>("opersonly") && !u->HasMode("OPER"))
		{
			u->SendMessage(bi, ACCESS_DENIED);
			return EVENT_STOP;
		}
		return EVENT_CONTINUE;
	}

	EventReturn OnPreHelp(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		if (!params.empty() || source.c || source.service != *RpgServ)
		{
			return EVENT_CONTINUE;
		}
		source.Reply(_("%s commands:"), RpgServ->nick.c_str());
		return EVENT_CONTINUE;
	}
	void OnLog(Log *l) anope_override
	{
		if (l->type == LOG_SERVER)
			l->bi = RpgServ;
	}
};

MODULE_INIT(RpgServCore)

