// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
 
#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <cstdlib>
#include <map>
#include <stdexcept>
class Calculator;
/*
   Represents a 'token' in an RPN expression. 
   An RPN expression looks something like:
     2 3 4 + *
   This class provides a common interface for handling both
   operators and operands.
 */
struct TokenBase { 
    virtual void evaluate (Calculator *) = 0; 
    virtual ~TokenBase() {}
};
/*
   Concrete 'token' of an RPN expression. 
   Operators are of type Token< char >
   Operands are of type Token< double >
 */
template< class T > class Token : public TokenBase {
    T token_;
public:
    /* Allow a calculator to consume this token */
    void evaluate (Calculator  *c);
    Token (T t) : token_(t) {}
};
/*
   Represents an expression in Reverse Polish Notation.
   This object basically acts as a FIFO queue of tokens
 */
class RPNExpression {
    std::vector< TokenBase* > stack_;
public:
    /* Add a token to the end of the expression */
    void push (TokenBase *t) { stack_.push_back (t); }
    /* Grab the next token from the front of the expression */
    TokenBase* pop () {
        TokenBase *t = stack_.front ();
        stack_.erase (stack_.begin ());
        return t;
    }
    bool empty () const { return stack_.empty (); }
};
/*
   Convert an expression in infix format to RPN format
 */
class ShuntingYard {
    const std::string expr_;
    RPNExpression rpn_;
    std::stack< char > op_stack_;
    mutable std::map< char, int > op_precedence_;
    /* Returns a precedence value for the given operator */
    int precedence (char op) const { return op_precedence_[op]; }
    /* Returns the precedence of the top item in the stack */
    int stack_precedence () const { 
        if (op_stack_.empty ()) { return -1; }
        return precedence (op_stack_.top ());
    }
    /* Reset precedence to allow for new scope */
    void handle_left_paren () { op_stack_.push ('('); }
    /* Consume all operators in current scope and restore previous scope */
    void handle_right_paren () {
        while ('(' != op_stack_.top ()) {
            rpn_.push (new Token< char >(op_stack_.top ()));
            op_stack_.pop ();
        }
        op_stack_.pop ();
    }
    /* Consume operators with precedence >= than op then add op */
    void handle_op (char op) {
        while (! op_stack_.empty () &&
                precedence (op) <= stack_precedence ()) {
            rpn_.push (new Token< char >(op_stack_.top ()));
            op_stack_.pop ();
        }
        op_stack_.push(op);
    }
    /* Convert infix to RPN via shunting-yard algorithm */
    RPNExpression convert(const std::string &infix) {
        const char * token = infix.c_str ();
        while (token && *token) {
            while (*token && isspace (*token)) { ++token; }
            if (! *token) { break; }
            if (isdigit (*token)) {
                char * next_token = 0;
                rpn_.push (new Token< double >(strtod (token, &next_token)));
                token = next_token;
            } else {
                char op = *token;
                switch (op) {
                    case '(':
                        handle_left_paren ();
                        break;
                    case ')':
                        handle_right_paren ();
                        break;
                    default:
                        handle_op (op);
                }
                ++token;
            }
        }
        while (! op_stack_.empty ()) {
            rpn_.push (new Token< char >(op_stack_.top ()));
            op_stack_.pop ();
        }
        return rpn_;
    }
public:
    ShuntingYard (const std::string& infix) : expr_(infix) {
        op_precedence_['('] = -1;
        op_precedence_['+'] = 2; op_precedence_['-'] = 2;
        op_precedence_['*'] = 3; op_precedence_['/'] = 3;
    }
    RPNExpression to_rpn () { return convert (expr_); }
};
/*
   A calculator that evaluates expressions by first converting them to
   reverse polish notation then processing the result.
 */
class Calculator {
    std::stack< double > operands_;
    double pop () { 
        double d = operands_.top ();
        operands_.pop ();
        return d; 
    }
    void push (double d) { operands_.push (d); }
    /* Returns the most recent operation result (top of the operand stack) */
    double result () const { return operands_.top (); }
    /* Empty the operand stack */
    void flush () { 
        while (! operands_.empty ()) { operands_.pop (); }
    }
protected:
    /* Process an operand token from the input stream */
    void consume(double value) { push (value); } 
    /* Process an operator token from the input stream */
    void consume(char op) { 
        switch (op) {
            case '+':
                push (pop () + pop ());
                break;
            case '*':
                push (pop () * pop ());
                break;
            case '-':
                {
                    double right = pop ();
                    push (pop () - right);
                }
                break;
            case '/':
                {
                    double right = pop ();
                    push (pop () / right);
                }
                break;
            default:
                throw std::domain_error("Unknown Operator");
        }
    } 
public:
    /* 
        Evaluate expression 
        Note: Expression is expected to be in infix form.
     */
    double calculate (const std::string& expr) { 
        ShuntingYard shunting(expr);
        RPNExpression rpn = shunting.to_rpn ();
        flush ();
        while (! rpn.empty ()) { 
            TokenBase * token = rpn.pop ();
            token->evaluate (this);
            delete token;
        }
        return result ();
    }
    /* Expose the consume() methods to the Tokens */
    template< class T > friend class Token;
};
template< class T > 
void Token< T >::evaluate (Calculator * c) { c->consume (token_); }
int main () {
    Calculator c;
    std::cout << c.calculate ("(20+10)*3/2-3") << std::endl;
    return 0;
}
