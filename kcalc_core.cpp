/*
    $Id$

    kCalculator, a scientific calculator for the X window system using the
    Qt widget libraries, available at no cost at http://www.troll.no

    The stack engine conatined in this file was take from
    Martin Bartlett's xfrmcalc

    portions:	Copyright (C) 1996 Bernd Johannes Wuebben
                                   wuebben@math.cornell.edu

    portions: 	Copyright (C) 1995 Martin Bartlett

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/



#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>

using namespace std;

#include <klocale.h>
#include <kmessagebox.h>
#include <knotifyclient.h>

#include "kcalc.h"
#include "dlabel.h"

#define UNUSED(x) ((void)(x))

#ifndef HAVE_FUNC_ISINF
	#ifdef HAVE_IEEEFP_H
		#include <ieeefp.h>
	#else
		#include <math.h>
#endif

int isinf(double x) { return !finite(x) && x==x; }
#endif

// TODO: used in global functions, need to work around that
// and no, static members is not a good idea!
bool display_error	= false;
bool percent_mode	= false;

stack_ptr	top_of_stack = NULL;	
stack_ptr	top_type_stack[2] = { NULL, NULL };	
int 		stack_next, stack_last;
stack_item	process_stack[STACK_SIZE];

item_contents 	display_data;

int precedence[14] = { 0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 6 };

int adjust_op[14][3] = 
{
	{FUNC_NULL,     FUNC_NULL,     FUNC_NULL},
	{FUNC_OR,       FUNC_OR,       FUNC_XOR },
	{FUNC_XOR,      FUNC_XOR,      FUNC_XOR },
	{FUNC_AND,      FUNC_AND,      FUNC_AND },
	{FUNC_LSH,      FUNC_LSH,      FUNC_RSH },
	{FUNC_RSH,      FUNC_RSH,      FUNC_RSH },
	{FUNC_ADD,      FUNC_ADD,      FUNC_ADD },
	{FUNC_SUBTRACT, FUNC_SUBTRACT, FUNC_SUBTRACT},
	{FUNC_MULTIPLY, FUNC_MULTIPLY, FUNC_MULTIPLY},
	{FUNC_DIVIDE,   FUNC_DIVIDE,   FUNC_DIVIDE},
	{FUNC_MOD,      FUNC_MOD,      FUNC_INTDIV },
	{FUNC_POWER,    FUNC_POWER,    FUNC_PWR_ROOT},
	{FUNC_PWR_ROOT, FUNC_PWR_ROOT, FUNC_PWR_ROOT},
	{FUNC_INTDIV,   FUNC_INTDIV,   FUNC_INTDIV},
};

/*
char *function_desc[] = 
{
	"Null",
	"Or",
	"Exclusive Or",
	"And",
	"Left Shift",
	"Right Shift",
	"Add",
	"Subtract",
	"Multiply",
	"Divide",
	"Modulus"
	"Power"
	"Reciprocal Power"
	"Integer Division"
};
*/

Arith Arith_ops[14] = 
{
	NULL,
	QtCalculator::ExecOr,
	QtCalculator::ExecXor,  	
	QtCalculator::ExecAnd,
	QtCalculator::ExecLsh,
	QtCalculator::ExecRsh,
	QtCalculator::ExecAdd, 
	QtCalculator::ExecSubtract,
	QtCalculator::ExecMultiply,
	QtCalculator::ExecDivide, 
	QtCalculator::ExecMod,
	QtCalculator::ExecPower, 
	QtCalculator::ExecPwrRoot,
	QtCalculator::ExecIntDiv
};	

Prcnt Prcnt_ops[14] = 
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	QtCalculator::ExecAddSubP,
	QtCalculator::ExecAddSubP,
	QtCalculator::ExecMultiplyP,
	QtCalculator::ExecDivideP,
	QtCalculator::ExecDivideP,
	QtCalculator::ExecPowerP,
	QtCalculator::ExecPwrRootP,
	QtCalculator::ExecDivideP
};
	

//-------------------------------------------------------------------------	
// Name: button0()
//-------------------------------------------------------------------------
void QtCalculator::button0()
{
	EnterDigit(0x0);
}

//-------------------------------------------------------------------------	
// Name: button1()
//-------------------------------------------------------------------------
void QtCalculator::button1()
{
	EnterDigit(0x1);
}

//-------------------------------------------------------------------------	
// Name: button2()
//-------------------------------------------------------------------------
void QtCalculator::button2()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x2);
}

//-------------------------------------------------------------------------	
// Name: button3()
//-------------------------------------------------------------------------
void QtCalculator::button3()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x3);
}

//-------------------------------------------------------------------------	
// Name: button4()
//-------------------------------------------------------------------------
void QtCalculator::button4()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x4);
}

//-------------------------------------------------------------------------	
// Name: button5()
//-------------------------------------------------------------------------
void QtCalculator::button5()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x5);
}

//-------------------------------------------------------------------------	
// Name: button6()
//-------------------------------------------------------------------------
void QtCalculator::button6()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x6);
}

//-------------------------------------------------------------------------	
// Name: button7()
//-------------------------------------------------------------------------
void QtCalculator::button7()
{
	if (current_base != NB_BINARY)
		EnterDigit(0x7);
}

//-------------------------------------------------------------------------	
// Name: button8()
//-------------------------------------------------------------------------
void QtCalculator::button8()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL))
		EnterDigit(0x8);
}

//-------------------------------------------------------------------------	
// Name: button9()
//-------------------------------------------------------------------------
void QtCalculator::button9()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL))
		EnterDigit(0x9);
}

//-------------------------------------------------------------------------	
// Name: buttonA()
//-------------------------------------------------------------------------
void QtCalculator::buttonA()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xA);  
}

//-------------------------------------------------------------------------	
// Name: buttonB()
//-------------------------------------------------------------------------
void QtCalculator::buttonB()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xB);  
}

//-------------------------------------------------------------------------	
// Name: buttonC()
//-------------------------------------------------------------------------
void QtCalculator::buttonC()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xC);  
}

//-------------------------------------------------------------------------	
// Name: buttonD()
//-------------------------------------------------------------------------
void QtCalculator::buttonD()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xD);  
}

//-------------------------------------------------------------------------	
// Name: buttonE()
//-------------------------------------------------------------------------
void QtCalculator::buttonE()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xE);  
}

//-------------------------------------------------------------------------	
// Name: buttonF()
//-------------------------------------------------------------------------
void QtCalculator::buttonF()
{
	if ((current_base != NB_BINARY) && (current_base != NB_OCTAL) &&
		(current_base != NB_DECIMAL))
		EnterDigit(0xF);  
}

//-------------------------------------------------------------------------
// Name: InitializeCalculator()
//-------------------------------------------------------------------------
void QtCalculator::InitializeCalculator()
{
	//
	// Basic initialization involves initializing the calcultion
	// stack, forcing the display to refresh to zero, and setting
	// up the floating point excetion signal handler to trap the
	// errors that the code can/has not been written to trap.
	//

	display_data.s_item_type = ITEM_AMOUNT;
	display_data.s_item_data.item_amount = 0.0;
	display_data.s_item_data.item_func_data.item_function = 0;
	display_data.s_item_data.item_func_data.item_precedence = 0;

	void fpe_handler(int fpe_parm);
	struct sigaction fpe_trap;

	fpe_trap.sa_handler = &fpe_handler;
#ifdef SA_RESTART
	fpe_trap.sa_flags = SA_RESTART;
#endif

	sigaction(SIGFPE, &fpe_trap, NULL);
	
	RefreshCalculator();
	
}		


//-------------------------------------------------------------------------
// Name: (int fpe_parm)
//-------------------------------------------------------------------------
void fpe_handler(int fpe_parm)
{
	UNUSED(fpe_parm);
	display_error = true;
	DISPLAY_AMOUNT = 0L;
}


//-------------------------------------------------------------------------
// Name: RefreshCalculator()
//-------------------------------------------------------------------------
void QtCalculator::RefreshCalculator()
{
	InitStack();
	display_error = false;
	DISPLAY_AMOUNT = 0L;
	inverse = false;
	UpdateDisplay();
	last_input = DIGIT; // must set last to DIGIT after Update Display in order
						// not to get a display holding e.g. 0.000
	input_count = 0;
	decimal_point = 0;
}		

//-------------------------------------------------------------------------
// Name: EnterDigit(int data)
//-------------------------------------------------------------------------
void QtCalculator::EnterDigit(int data)
{
	if(eestate)
	{
		QString string;
		string.setNum(data);
		strcat(display_str, string.latin1());
		DISPLAY_AMOUNT = (CALCAMNT) strtod(display_str,0);
		UpdateDisplay();
		return;
	}

	last_input = DIGIT;
	
	if (refresh_display)
	{
		DISPLAY_AMOUNT = 0L;
		decimal_point = 0;
		refresh_display = false;
		input_count = 0;
	}

	if (!(input_limit && input_count >= input_limit))
		if (DISPLAY_AMOUNT < 0)
			DISPLAY_AMOUNT = decimal_point ?
			DISPLAY_AMOUNT - ((CALCAMNT)data /
			POW(current_base, decimal_point++)) :
			(current_base * DISPLAY_AMOUNT) - data;
		else
			DISPLAY_AMOUNT = decimal_point ?
			DISPLAY_AMOUNT + ((CALCAMNT)data /
			POW(current_base, decimal_point++)) :
			(current_base * DISPLAY_AMOUNT) + data;

		if (decimal_point)
		{
			input_count ++;
#ifdef MYDEBUG
	printf("EnterDigit() inc dec.point:%d\n",input_count);
#endif
		}
	
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: SubtractDigit()
//-------------------------------------------------------------------------
void QtCalculator::SubtractDigit()
{
	// This function could be better, possibly, but am I glad to see it!
	if (DISPLAY_AMOUNT != 0)
	{
                if (current_base == NB_DECIMAL && (DISPLAY_AMOUNT != floor(DISPLAY_AMOUNT)))
                {
                        if (decimal_point < 3)
                        {
                            decimal_point = 0;
                            DISPLAY_AMOUNT = floor(DISPLAY_AMOUNT);
                        }
                        else
                        {
                            --decimal_point;
                            DISPLAY_AMOUNT = floor(DISPLAY_AMOUNT * POW(current_base, decimal_point - 1)) / 
                                             POW(current_base, (decimal_point - 1));
                        }
                }
                else
                {
                        DISPLAY_AMOUNT = floor(DISPLAY_AMOUNT / current_base);
                }
                
                if (input_count > 0)
                {
                        --input_count;
                }
	}

#ifdef MYDEBUG
	printf("SubtractDigit()");
#endif
 
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: EnterDecimal()
//-------------------------------------------------------------------------
void QtCalculator::EnterDecimal()
{
	// exit if we aren't in decimla mode
	if (current_base != NB_DECIMAL)
		return;

	if(eestate)
	{
		KNotifyClient::beep();
		return;
	}

	
	if (refresh_display)
	{
		DISPLAY_AMOUNT = 0L;
		refresh_display = false;
		input_count = 0;
	}
	
	if(last_input == DIGIT)
	{
		if (!strpbrk(display_str,"."))
		{
			// if the last input was a DIGIT and we don't 
			// have already a period in our
			// display string then display a period

			calc_display->setText(strcat(display_str, "."));
			decimal_point = 1;
		}
	}
	else
	{
		// the last input wasn't a DIGIT so we are about to
		// input a new number in particular we neet do display a "0.".

		if (!strpbrk(display_str,"."))
		{
		
			DISPLAY_AMOUNT = 0L;
			refresh_display = false;
			decimal_point = 1;

			//	  decimal_point = 1;
			//	  input_count = 1;

			strcpy(display_str, "0.");
			calc_display->setText(display_str);
		}
	}
}

//-------------------------------------------------------------------------
// Name: Or()
//-------------------------------------------------------------------------
void QtCalculator::Or()
{
	eestate = false;
	if (inverse)
	{
		EnterStackFunction(2);   // XOR
		inverse = false;
	}
	else
		EnterStackFunction(1);   // OR

	last_input = OPERATION;
}

//-------------------------------------------------------------------------
// Name: And()
//-------------------------------------------------------------------------
void QtCalculator::And()
{
	eestate = false;
	last_input = OPERATION;
	EnterStackFunction(3);
}


//-------------------------------------------------------------------------
// Name: Shift()
//-------------------------------------------------------------------------
void QtCalculator::Shift()
{
	eestate = false;
	last_input = OPERATION;
	if (inverse)
	{
		EnterStackFunction(5);   // Rsh
		inverse = false;
	}
	else
		EnterStackFunction(4);   // Lsh
}

//-------------------------------------------------------------------------
// Name: Plus()
//-------------------------------------------------------------------------
void QtCalculator::Plus()
{
	eestate = false;
	last_input = OPERATION;
	EnterStackFunction(6);
}

//-------------------------------------------------------------------------
// Name: Minus()
//-------------------------------------------------------------------------
void QtCalculator::Minus()
{
	eestate = false;
	last_input = OPERATION;
	EnterStackFunction(7);
}

//-------------------------------------------------------------------------
// Name: Multiply()
//-------------------------------------------------------------------------
void QtCalculator::Multiply()
{
	eestate = false;
	last_input = OPERATION;
	EnterStackFunction(8);
}

//-------------------------------------------------------------------------
// Name: Divide()
//-------------------------------------------------------------------------
void QtCalculator::Divide()
{
	eestate = false;
	last_input = OPERATION;
	EnterStackFunction(9);
}

//-------------------------------------------------------------------------
// Name: Mod()
//-------------------------------------------------------------------------
void QtCalculator::Mod()
{
	eestate = false;
	last_input = OPERATION;
	if (inverse)
	{
		EnterStackFunction(13);   // InvMod
		inverse = false;
	}
	else 
	{
		EnterStackFunction(10);   // Mod
	}
}

//-------------------------------------------------------------------------
// Name: Power()
//-------------------------------------------------------------------------
void QtCalculator::Power()
{
	eestate = false;
	last_input = OPERATION;
	if (inverse)
	{
		EnterStackFunction(12);   // InvPower
		inverse = false;
	}
	else
	{
		EnterStackFunction(11);   // Power
	}
}


//-------------------------------------------------------------------------
// Name: EnterStackFunction(int data)
//-------------------------------------------------------------------------
void QtCalculator::EnterStackFunction(int data)
{
	item_contents 	new_item;
	int new_precedence;
	int dummy = 0;

	/*
	if (inverse ) {
	  dummy = 3;
	  inverse = false;
	}
	else {
	  dummy = 1;
	}
	*/

	//	printf("data %d dummy %d\n",data,dummy);
	data = adjust_op[data][dummy];
	//	printf("data %d \n",data );

	PushStack(&display_data);

	new_item.s_item_type = ITEM_FUNCTION;
	new_item.s_item_data.item_func_data.item_function = data;
	new_item.s_item_data.item_func_data.item_precedence =
		new_precedence = precedence[data] + precedence_base;

	refresh_display = true;
	if (UpdateStack(new_precedence))
		UpdateDisplay();
	PushStack(&new_item);
}

//-------------------------------------------------------------------------
// Name: EnterNegate()
//-------------------------------------------------------------------------
void QtCalculator::EnterNegate()
{
	if(eestate)
	{
		QString str(display_str);
		size_t pos = str.findRev('e');
		
		if(pos == -1)
			return;
			
		if(display_str[pos+1] == '+')
			display_str[pos+1] = '-';
		else 
		{
			if(display_str[pos+1] == '-')
				display_str[pos+1] = '+';
			else
			{
				str.insert(pos + 1, "-");
				strncpy(display_str, str.latin1(), DSP_SIZE);
			}
		}			
		
		DISPLAY_AMOUNT = (CALCAMNT)strtod(display_str,0);
		UpdateDisplay();
	}
	else
	{
		//    last_input = OPERATION;
		if (DISPLAY_AMOUNT != 0)
		{
			DISPLAY_AMOUNT *= -1;
			UpdateDisplay();
		}
	}
	
	last_input = DIGIT;
}

//-------------------------------------------------------------------------
// Name: EnterOpenParen()
//-------------------------------------------------------------------------
void QtCalculator::EnterOpenParen()
{
	eestate = false;
	last_input = OPERATION;
	precedence_base += PRECEDENCE_INCR;
	refresh_display = true;
}

//-------------------------------------------------------------------------
// Name: EnterCloseParen()
//-------------------------------------------------------------------------
void QtCalculator::EnterCloseParen()
{
	eestate = false;
	last_input = OPERATION;
	PushStack(&display_data);
	refresh_display = true;

	if (UpdateStack(precedence_base))
		UpdateDisplay();
		
	if ((precedence_base -= PRECEDENCE_INCR) < 0)
		precedence_base = 0;
}

//-------------------------------------------------------------------------
// Name: EnterRecip()
//-------------------------------------------------------------------------
void QtCalculator::EnterRecip()
{
	eestate = false;
	last_input = OPERATION;
	DISPLAY_AMOUNT = 1 / DISPLAY_AMOUNT;	
	refresh_display = true;
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: EnterInt()
//-------------------------------------------------------------------------
void QtCalculator::EnterInt()
{
	eestate = false;
	CALCAMNT work_amount1 = 0;
	CALCAMNT work_amount2 = 0;

	last_input = OPERATION;	
	
	if (!inverse)
	{
		work_amount2 = MODF(DISPLAY_AMOUNT, &work_amount1);
		DISPLAY_AMOUNT = work_amount2 ;
	}
	else
	{
		DISPLAY_AMOUNT = work_amount1;
		inverse = false;
	}

	refresh_display = true;
	UpdateDisplay();

}

//-------------------------------------------------------------------------
// Name: EnterFactorial()
//-------------------------------------------------------------------------
void QtCalculator::EnterFactorial()
{
	eestate = false;
	CALCAMNT work_amount1;
	CALCAMNT work_amount2;
	int incr;

	MODF(DISPLAY_AMOUNT, &work_amount1);

	incr = work_amount1 < 0 ? -1 : 1;
	
	work_amount2 = work_amount1 - incr;
	
	while (work_amount1 != 0 && work_amount2 != 0 && !display_error)
	{
		work_amount1 *= work_amount2;
		work_amount2 -= incr;
		if(ISINF(work_amount1))
		{
			display_error = true;
			break;
		}
	}

	if(work_amount1 == 0.0)
		work_amount1 = 1.0;

	DISPLAY_AMOUNT = work_amount1;
	refresh_display = true;
	last_input = OPERATION;
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: EnterSquare()
//-------------------------------------------------------------------------
void QtCalculator::EnterSquare()
{
	eestate = false;

	if (!inverse)					DISPLAY_AMOUNT *= DISPLAY_AMOUNT;
	else if (DISPLAY_AMOUNT < 0)	display_error = true;
	else							DISPLAY_AMOUNT = SQRT(DISPLAY_AMOUNT);
		
	refresh_display = true;
	inverse = false;
	last_input = OPERATION;	
	UpdateDisplay();

}

//-------------------------------------------------------------------------
// Name: EnterNotCmp()
//-------------------------------------------------------------------------
void QtCalculator::EnterNotCmp()
{
	eestate = false;
	CALCAMNT boh_work_d;
	long boh_work;

	MODF(DISPLAY_AMOUNT, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
		display_error = true;
	else 
	{
		boh_work = (long int)boh_work_d;
		DISPLAY_AMOUNT = ~boh_work;	
	}
	
	refresh_display = true;
	last_input = OPERATION;
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: EnterHyp()
//-------------------------------------------------------------------------
void QtCalculator::EnterHyp()
{
	switch(kcalcdefaults.style)
	{
	case 1:
		if(!inverse)
		{
			eestate = false; // terminate ee input mode
			DISPLAY_AMOUNT =  stats.count();
		}
		else
		{
			inverse = false;
			eestate = false; // terminate ee input mode
			DISPLAY_AMOUNT =  stats.sum();
		}
		
		last_input = OPERATION;
		refresh_display = true;
		UpdateDisplay();		
		break;
		
	case 0:
		// toggle between hyperbolic and standart trig functions
		hyp_mode = !hyp_mode;

		if (hyp_mode)	statusHYPLabel->setText("HYP");
		else 			statusHYPLabel->clear();
		break;
	}
}

//-------------------------------------------------------------------------
// Name: ExecSin()
//-------------------------------------------------------------------------
void QtCalculator::ExecSin()
{
	switch(kcalcdefaults.style)
	{
	case 0:
		// trig mode
		ComputeSin();
		break;
	
	case 1:
		// stats mode
		ComputeMean();
		break;
	}
}

//-------------------------------------------------------------------------
// Name: ComputeMean()
//-------------------------------------------------------------------------
void QtCalculator::ComputeMean()
{
	if(!inverse)
	{
		eestate = false;
		DISPLAY_AMOUNT = stats.mean();
	
		if (stats.error())
			display_error = true;

		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();
	}
	else
	{
		inverse = false;
		eestate = false;
		DISPLAY_AMOUNT = stats.sum_of_squares();
	
		if (stats.error())
			display_error = true;

		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();
	}
}

//-------------------------------------------------------------------------
// Name: ComputeSin()
//-------------------------------------------------------------------------
void QtCalculator::ComputeSin()
{
	CALCAMNT work_amount = DISPLAY_AMOUNT;
	eestate = false;

	if (hyp_mode)
	{
		// sinh or arcsinh
		if (!inverse)
		{
			DISPLAY_AMOUNT = SINH( work_amount);
		}
		else
		{
			DISPLAY_AMOUNT = ASINH( work_amount);
		
			if (errno == EDOM || errno == ERANGE)
				display_error = true;
	
			// reset the inverse flag
			inverse = false;
		}
	}
	else 
	{
		// sine or arcsine
		if (!inverse)
		{
			// sine
			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Deg2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}
	
			DISPLAY_AMOUNT = SIN( work_amount);
		}
		else
		{
			// arcsine
			DISPLAY_AMOUNT = ASIN(work_amount);
			
			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Rad2Deg(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Rad2Gra(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}
	
			DISPLAY_AMOUNT = work_amount;
		
			if (errno == EDOM || errno == ERANGE)
				display_error = true;

			// reset the inverse flag
			inverse = false;
		}
	}

	// Now a cheat to help the weird case of COS 90 degrees not being 0!!!
	if (DISPLAY_AMOUNT < POS_ZERO && DISPLAY_AMOUNT > NEG_ZERO)
		DISPLAY_AMOUNT = 0;
		
	refresh_display = true;
	last_input = OPERATION;
	UpdateDisplay();

}

//-------------------------------------------------------------------------
// Name: ExecCos()
//-------------------------------------------------------------------------
void QtCalculator::ExecCos()
{
	switch(kcalcdefaults.style)
	{
	case 0:
		// trig mode
		ComputeCos();
		break;
	
	case 1:
		// stats mode
		ComputeStd();
		break;
	}
}

//-------------------------------------------------------------------------
// Name: ComputeStd()
//-------------------------------------------------------------------------
void QtCalculator::ComputeStd()
{
	if(!inverse)
	{
		// std (n-1)
		inverse = false;
		eestate = false;
		DISPLAY_AMOUNT = stats.std();

		if (stats.error())
			display_error = true;
			
		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();
	}
	else
	{
		// std (n)
		inverse = false;
		eestate = false;
		DISPLAY_AMOUNT = stats.sample_std();

		if (stats.error())
			display_error = true;

		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();
	}
}

//-------------------------------------------------------------------------
// Name: ComputeCos()
//-------------------------------------------------------------------------
void QtCalculator::ComputeCos()
{
	CALCAMNT work_amount = DISPLAY_AMOUNT;
	eestate = false;

	if (hyp_mode)
	{
		// cosh or arccosh
		if (!inverse)
		{
			DISPLAY_AMOUNT = COSH(work_amount);
		}
		else
		{
			DISPLAY_AMOUNT = ACOSH(work_amount);
			
			if (errno == EDOM || errno == ERANGE)
				display_error = true;

			inverse = false;       // reset the inverse flag
		}
	}
	else
	{
		// cosine or arccosine
		if (!inverse)
		{
			// cosine
			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Deg2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}
			
			DISPLAY_AMOUNT = COS(work_amount);
		}
		else
		{
			// arccosine
			DISPLAY_AMOUNT = ACOS(work_amount);

			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Deg2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}
			
			DISPLAY_AMOUNT = work_amount;

			if (errno == EDOM || errno == ERANGE)
				display_error = true;

			// reset the inverse flag
			inverse = false;
		}
	}

	// Now a cheat to help the weird case of COS 90 degrees not being 0!!!
	if (DISPLAY_AMOUNT < POS_ZERO && DISPLAY_AMOUNT > NEG_ZERO)
		DISPLAY_AMOUNT = 0;

	refresh_display = true;
	last_input = OPERATION;
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: ComputeMedean()
//-------------------------------------------------------------------------
void QtCalculator::ComputeMedean()
{
	if(!inverse)
	{
		// std (n-1)
		inverse = false;
		eestate = false;
		DISPLAY_AMOUNT = stats.median();

		if (stats.error())
			display_error = true;

		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();
	}
	else
	{
		// std (n)
		inverse = false;
		eestate = false;
		DISPLAY_AMOUNT = stats.median();

		if (stats.error())
			display_error = true;

		refresh_display = true;
		last_input = OPERATION;
		UpdateDisplay();

	}
}

//-------------------------------------------------------------------------
// Name: ComputeTan()
//-------------------------------------------------------------------------
void QtCalculator::ComputeTan()
{
	CALCAMNT work_amount = DISPLAY_AMOUNT;
	eestate = false;

	if (hyp_mode)
	{
		// tanh or arctanh
		if (!inverse)
		{
			DISPLAY_AMOUNT = TANH( work_amount);
		}
		else
		{
			DISPLAY_AMOUNT = ATANH( work_amount);
			if (errno == EDOM || errno == ERANGE)
				display_error = true;
				
			// reset the inverse flag
			inverse = false;
		}
	}
	else
	{
		// tan or arctan
		if (!inverse)
		{
			// tan
			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Deg2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}
			
			DISPLAY_AMOUNT = TAN(work_amount);
	
			if (errno == EDOM || errno == ERANGE)
				display_error = true;
		}
		else
		{
			// arctan
			DISPLAY_AMOUNT = ATAN(work_amount);

			switch (angle_mode)
			{
			case ANG_DEGREE:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_GRADIENT:
				work_amount = QtCalculator::Gra2Rad(DISPLAY_AMOUNT);
				break;
			case ANG_RADIAN:
				work_amount = DISPLAY_AMOUNT;
				break;
			}

			DISPLAY_AMOUNT = work_amount;

			if (errno == EDOM || errno == ERANGE)
				display_error = true;
	
			// reset the inverse flag
			inverse = false;
		}
	}

	// Now a cheat to help the weird case of COS 90 degrees not being 0!!!

	if (DISPLAY_AMOUNT < POS_ZERO && DISPLAY_AMOUNT > NEG_ZERO)
		DISPLAY_AMOUNT = 0;
		
	refresh_display = true;
	last_input = OPERATION;	
	UpdateDisplay();

}

//-------------------------------------------------------------------------
// Name: ExecTan()
//-------------------------------------------------------------------------
void QtCalculator::ExecTan()
{
	switch(kcalcdefaults.style)
	{
	case 0:
		// trig mode
		ComputeTan();	
		break;
	
	case 1:
		// stats mode
		ComputeMedean();
		break;
	}
}

//-------------------------------------------------------------------------
// Name: EnterPercent()
//-------------------------------------------------------------------------
void QtCalculator::EnterPercent()
{
	eestate			= false;
	last_input		= OPERATION;
	percent_mode	= true;
	EnterEqual();
	percent_mode	= false;

}

//-------------------------------------------------------------------------
// Name: EnterLogr()
//-------------------------------------------------------------------------
void QtCalculator::EnterLogr()
{
	switch(kcalcdefaults.style)
	{
	case 1:
		if(!inverse)
		{
			eestate = false; // terminate ee input mode
			stats.enterData(DISPLAY_AMOUNT);
			DISPLAY_AMOUNT = stats.count();
		}
		else
		{
			inverse = false;
			stats.clearLast();
			setStatusLabel(i18n("Last stat item erased"));
			DISPLAY_AMOUNT = stats.count();
		}
		break;
		
	case 0:
		eestate = false;		
		if (!inverse)
		{
			if (DISPLAY_AMOUNT <= 0)
				display_error = true;
			else
				DISPLAY_AMOUNT = LOG_TEN(DISPLAY_AMOUNT);	
		}
		else
		{
			DISPLAY_AMOUNT = POW(10, DISPLAY_AMOUNT);
			inverse = false;
		}	
		break;
	}

	last_input		= OPERATION;
	refresh_display	= true;
	UpdateDisplay();
}

//-------------------------------------------------------------------------
// Name: EnterLogn()
//-------------------------------------------------------------------------
void QtCalculator::EnterLogn()
{
	switch(kcalcdefaults.style)
	{
	case 1:
		if(!inverse)
		{
			stats.clearAll();
			setStatusLabel(i18n("Stat Mem cleared"));
		}
		else
		{
			inverse = false;
			UpdateDisplay();
		}
		break;

	case 0:
		eestate = false;
		last_input = OPERATION;
		if (!inverse)
		{
			if (DISPLAY_AMOUNT <= 0)
				display_error = true;
			else
				DISPLAY_AMOUNT = LOG(DISPLAY_AMOUNT);				
		}
		else if (inverse)
		{
			DISPLAY_AMOUNT = EXP(DISPLAY_AMOUNT);	
			inverse = false;
		}
		
		refresh_display = true;			
		UpdateDisplay();
		break;
	}
}

//-------------------------------------------------------------------------
// Name: base_selected(int number)
//-------------------------------------------------------------------------
void QtCalculator::base_selected(int number)
{
	switch(number)
	{
	case 0:	
		current_base	= NB_HEX;
		display_size	= HEX_SIZE;
		decimal_point	= 0;
		input_limit		= 8;
		break;
	case 1:	
		current_base	= NB_DECIMAL;
		display_size	= DEC_SIZE;
		input_limit		= 0;
		break;
	case 2:
		current_base	= NB_OCTAL;
		display_size	= OCT_SIZE;
		decimal_point	= 0;
		input_limit		= 11;
		break;
	case 3:
		current_base	= NB_BINARY;
		display_size	= BIN_SIZE;
		decimal_point	= 0;
		input_limit		= 32;
		break;
	default: // we shouldn't ever end up here
		current_base	= NB_DECIMAL;
		display_size	= DEC_SIZE;
		input_limit		= 0;
	}
	
	UpdateDisplay();
}


//-------------------------------------------------------------------------
// Name: angle_selected(int number)
//-------------------------------------------------------------------------
void QtCalculator::angle_selected(int number)
{
	switch(number)
	{
	case 0:	
		angle_mode = ANG_DEGREE;
		break;
	case 1:	
		angle_mode = ANG_RADIAN;
		break;
	case 2:
		angle_mode = ANG_GRADIENT;
		break;
	default: // we shouldn't ever end up here
		angle_mode = ANG_RADIAN;
	}
}

//-------------------------------------------------------------------------
// Name: SetInverse()
//-------------------------------------------------------------------------
void QtCalculator::SetInverse()
{
	inverse = ! inverse;
	
	if (inverse)	statusINVLabel->setText("INV");
	else			statusINVLabel->setText("NORM");
}

//-------------------------------------------------------------------------
// Name: EE()
//-------------------------------------------------------------------------
void QtCalculator::EE()
{
	if(inverse)
	{
		DISPLAY_AMOUNT	= pi;
		inverse			= false;
		refresh_display	= true;
	
		UpdateDisplay();
	}
	else if (refresh_display)
	{
		//TODO: toggle between exponential and normal display
	}	
	else
	{
		if(!eestate)
			strcat(display_str,"e");
		
		eestate = !eestate;

		UpdateDisplay();
	}
}

//-------------------------------------------------------------------------
// Name: MR()
//-------------------------------------------------------------------------
void QtCalculator::MR()
{
	eestate			= false;
	last_input		= OPERATION;
	DISPLAY_AMOUNT	= memory_num;
	refresh_display	= true;
	
	UpdateDisplay();

}

//-------------------------------------------------------------------------
// Name: Mplusminus()
//-------------------------------------------------------------------------
void QtCalculator::Mplusminus()
{
	eestate = false;
	EnterEqual();
	
	if (!inverse)	memory_num += DISPLAY_AMOUNT;
	else 			memory_num -= DISPLAY_AMOUNT;
		
	inverse = false;
}

//-------------------------------------------------------------------------
// Name: MC()
//-------------------------------------------------------------------------
void QtCalculator::MC()
{
	memory_num		= 0;
	refresh_display	= true;
}

//-------------------------------------------------------------------------
// Name: EnterEqual()
//-------------------------------------------------------------------------
void QtCalculator::EnterEqual()
{
	eestate		= false;
	last_input	= OPERATION;
	
	PushStack(&display_data);
	
	refresh_display = true;

	//if (UpdateStack(0))
	UpdateStack(0);

	UpdateDisplay();
	precedence_base = 0;

	// add this latest value to our history
	history_list.insert(history_list.begin(), DISPLAY_AMOUNT);
}

//-------------------------------------------------------------------------
// Name: Clear()
//-------------------------------------------------------------------------
void QtCalculator::Clear()
{
	
	eestate 		= false;
	input_count		= 0;
	decimal_point	= 0;

	
	if (last_input == OPERATION)
	{
		PopStack();
		last_input = DIGIT;
	}
	
	if(display_error)
	{
		display_error	= false;
		refresh_display	= false;
	}

	if (!refresh_display)
	{
		DISPLAY_AMOUNT = 0L;
		UpdateDisplay();
	}
	
	//RefreshCalculator();	
}

//-------------------------------------------------------------------------
// Name: ClearAll()
//-------------------------------------------------------------------------
void QtCalculator::ClearAll()
{
	eestate = false;
	
	// last_input = OPERATION;
	last_input = DIGIT;
	
	RefreshCalculator();
	refresh_display = true;
}


//-------------------------------------------------------------------------
// Name: UpdateDisplay()
//-------------------------------------------------------------------------
void QtCalculator::UpdateDisplay()
{

	// this needs to be rewritten based on whether we are currently
	// inputting a number so that the period and the 0 after a period
	// are correctly displayed.

	CALCAMNT	boh_work_d;
	long 		boh_work = 0;
	int			str_size = 0;

	if(eestate && (current_base == NB_DECIMAL))
	{
		calc_display->setText(display_str);
		return;
	}

	if (current_base != NB_DECIMAL)
	{
		MODF(DISPLAY_AMOUNT, &boh_work_d);

		if (boh_work_d < LONG_MIN || boh_work_d > ULONG_MAX)
			display_error = true;

		//
		// We may be in that never-never land where boh numbers
		// turn from positive to negative - if so then we do
		// just that, allowing boh negative numbers to be entered
		// as read (from dumps and the like!)
		//
		else if (boh_work_d > LONG_MAX)
		{
			DISPLAY_AMOUNT = LONG_MIN + (boh_work_d - LONG_MAX - 1);
			boh_work = (long)DISPLAY_AMOUNT;
		}
		else
		{
			DISPLAY_AMOUNT = boh_work_d;
			boh_work = (long)boh_work_d;
		}
	}

	if (!display_error)
	{
		switch(current_base)
		{
		case NB_BINARY:
			str_size = QtCalculator::cvb(display_str, boh_work, BIN_SIZE);
			break;
			
		case NB_OCTAL:
			str_size = sprintf(display_str, "%lo", boh_work);
			break;
			
		case NB_HEX:
			str_size = sprintf(display_str, "%lX", boh_work);
			break;
			
		case NB_DECIMAL:
                        if (kcalcdefaults.fixed)
                        {
                                str_size = sprintf(display_str,

#ifdef HAVE_LONG_DOUBLE
                                        "%.*Lf", // was *Lg
                                        kcalcdefaults.fixedprecision,
#else
                                        "%.*f",
                                        kcalcdefaults.fixedprecision,
#endif
                                        DISPLAY_AMOUNT);
                        }
			else if(last_input == DIGIT || DISPLAY_AMOUNT > 1.0e+16)
			{

				// if I don't guard against the DISPLAY_AMOUNT being too large
				// kcalc will segfault on larger amount. Such as from typing
				// from 5*5*******
				str_size = sprintf(display_str,
#ifdef HAVE_LONG_DOUBLE
					"%.*Lg", // was *Lg
					kcalcdefaults.precision + 1,
#else
					"%.*g",
					kcalcdefaults.precision + 1,
#endif
					DISPLAY_AMOUNT);
			}
                        else
                        {
#ifdef HAVE_LONG_DOUBLE
                            str_size = sprintf(display_str, "%Lg", DISPLAY_AMOUNT);
#else
                            str_size = sprintf(display_str, "%g", DISPLAY_AMOUNT);
#endif
                        }

			if (input_count > 0 && !strpbrk(display_str,"e") && 
				last_input == DIGIT )
			{ 
#ifdef HAVE_LONG_DOUBLE
				str_size = sprintf(display_str,
					"%.*Lf",
					(kcalcdefaults.precision +1 > input_count)?
					input_count : kcalcdefaults.precision ,
					DISPLAY_AMOUNT);
#else
				str_size = sprintf(display_str,
					"%.*f",
					(kcalcdefaults.precision +1 > input_count)?
					input_count : kcalcdefaults.precision ,
					DISPLAY_AMOUNT);
#endif
			}
			break;	

		default:
			display_error = true;
			break;
		}
	}

	if (display_error || str_size < 0)
	{
		display_error = true;
		strcpy(display_str, i18n("Error").utf8());
		
		if(kcalcdefaults.beep)
			KNotifyClient::beep();
	}

	if (inverse)	statusINVLabel->setText("INV");
	else			statusINVLabel->setText("NORM");

	if (hyp_mode)	statusHYPLabel->setText("HYP");
	else			statusHYPLabel->clear();
	calc_display->setText(display_str);
}

//-------------------------------------------------------------------------
// Name: cvb(char *out_str, long amount, int max_digits)
//-------------------------------------------------------------------------
int QtCalculator::cvb(char *out_str, long amount, int max_digits)
{
	/*
	* A routine that converts a long int to
	* binary display format
	*/

	/*
	char		work_str[(sizeof(amount) * CHAR_BIT) + 1];
	int		work_char = 0,
	lead_zero = 1,
	lead_one = 1,
	lead_one_count = 0,
	work_size = sizeof(amount) * CHAR_BIT;
	unsigned long	bit_mask = ((unsigned long) 1 << ((sizeof(amount) * CHAR_BIT) - 1));

	while (bit_mask) {

	if (amount & bit_mask) {
	if (lead_one)
	lead_one_count++;
	lead_zero = 0;
	work_str[work_char++] = '1';
	} else {
	lead_one = 0;
	if (!lead_zero)
	work_str[work_char++] = '0';
	}
	bit_mask >>= 1;
	bit_mask &= 0x7fffffff; //Sven: Uwe's Alpha adition
	}
	if (!work_char)
	work_str[work_char++] = '0';
	work_str[work_char] = '\0';

	if (work_char-lead_one_count < max_digits)
	return strlen(strcpy(out_str,
	&work_str[lead_one_count ?
	work_size - max_digits :
	0]));
	else
	return -1;
	*/
	
	char *strPtr	= out_str;
	bool hitOne		= false;
	unsigned long bit_mask = 
		((unsigned long) 1 << ((sizeof(amount) * CHAR_BIT) - 1));
	
	while(bit_mask != 0 && max_digits > 0)
	{
		char tmp = (bit_mask & amount) ? '1' : '0';
		
		if(!hitOne && tmp == '1')
			hitOne = true;
		
		if(hitOne)
			*strPtr++ = tmp;
		
		bit_mask >>= 1;

		// this will fix a prob with some processors using an 
		// arithmetic right shift (which would maintain sign on
		// negative numbers and cause a loop that's too long)
		bit_mask &= 0x7fffffff; //Sven: Uwe's Alpha adition
		
		max_digits--;
	}
	
	if(amount == 0)
		*strPtr++ = '0';
	
	*strPtr = '\0';
	
	return strlen(out_str);
}		

//-------------------------------------------------------------------------
// Name: UpdateStack(int run_precedence)
//-------------------------------------------------------------------------
int UpdateStack(int run_precedence)
{
	item_contents new_item;
	item_contents *top_item;
	item_contents *top_function;
	
	CALCAMNT left_op	= 0.0;
	CALCAMNT right_op 	= 0.0;
	int op_function		= 0;
	int return_value	= 0;

	new_item.s_item_type = ITEM_AMOUNT;
	
	while ((top_function = TopTypeStack(ITEM_FUNCTION)) &&
		top_function->s_item_data.item_func_data.item_precedence >=
		run_precedence)
	{
		return_value = 1;

		if ((top_item = PopStack())->s_item_type != ITEM_AMOUNT)
		{
			KMessageBox::error(0L, i18n("Stack processing error - right_op"));
		}
		
		right_op = top_item->s_item_data.item_amount;

		if (!((top_item = PopStack()) && 
			top_item->s_item_type == ITEM_FUNCTION))
		{
			KMessageBox::error(0L, i18n("Stack processing error - function"));
		}

		op_function = top_item->s_item_data.item_func_data.item_function;	

		if (!((top_item = PopStack()) && top_item->s_item_type == ITEM_AMOUNT))
			KMessageBox::error(0L, i18n("Stack processing error - left_op"));

		left_op = top_item->s_item_data.item_amount;

		new_item.s_item_data.item_amount = 
			(Arith_ops[op_function])(left_op, right_op);

		PushStack(&new_item);
	}
	
	if (return_value && 
		percent_mode &&
		!display_error &&
		Prcnt_ops[op_function] != NULL)
	{
		new_item.s_item_data.item_amount = 
			(Prcnt_ops[op_function])
				(left_op, right_op, new_item.s_item_data.item_amount);
				
		PushStack(&new_item);
	}
	
	if (return_value)
		DISPLAY_AMOUNT = new_item.s_item_data.item_amount;

	return return_value;
}

//-------------------------------------------------------------------------
// Name: isoddint(CALCAMNT input)
//-------------------------------------------------------------------------
int isoddint(CALCAMNT input)
{
	CALCAMNT	dummy;
	
	// Routine to check if CALCAMNT is an Odd integer
	return (MODF(input, &dummy) == 0.0 && MODF(input / 2, &dummy) == 0.5);
}

//-------------------------------------------------------------------------
// Name: ExecOr(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecOr(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecOr\n");
	CALCAMNT	boh_work_d;
	long 		boh_work_l;
	long		boh_work_r;

	MODF(left_op, &boh_work_d);
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_l = (long int)boh_work_d;
	MODF(right_op, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_r = (long int) boh_work_d;
	return (boh_work_l | boh_work_r);
}

//-------------------------------------------------------------------------
// Name: ExecXor(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecXor(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecXOr\n");
	CALCAMNT	boh_work_d;
	long 		boh_work_l;
	long		boh_work_r;

	MODF(left_op, &boh_work_d);
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_l = (long int)boh_work_d;
	MODF(right_op, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_r = (long int)boh_work_d;
	return (boh_work_l ^ boh_work_r);
}

//-------------------------------------------------------------------------
// Name: ExecAnd(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecAnd(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecAnd\n");
	CALCAMNT	boh_work_d;
	long 		boh_work_l;
	long		boh_work_r;

	MODF(left_op, &boh_work_d);
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_l = (long int)boh_work_d;
	MODF(right_op, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_r = (long int)boh_work_d;
	return (boh_work_l & boh_work_r);
}

//-------------------------------------------------------------------------
// Name: ExecLsh(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecLsh(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecLsh\n");
	CALCAMNT	boh_work_d;
	long 		boh_work_l;
	long		boh_work_r;

	MODF(left_op, &boh_work_d);
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_l = (long int) boh_work_d;
	MODF(right_op, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_r = (long int ) boh_work_d;
	return (boh_work_l << boh_work_r);
}

//-------------------------------------------------------------------------
// Name: ExecRsh(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecRsh(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecRsh\n");
	CALCAMNT	boh_work_d;
	long 		boh_work_l;
	long		boh_work_r;

	MODF(left_op, &boh_work_d);
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_l = (long int)boh_work_d;
	MODF(right_op, &boh_work_d);
	
	if (FABS(boh_work_d) > LONG_MAX)
	{
		display_error = true;
		return 0;
	}
	
	boh_work_r = (long int)boh_work_d;
	return (boh_work_l >> boh_work_r);
}

//-------------------------------------------------------------------------
// Name: ExecAdd(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecAdd(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecAdd\n");
	return left_op + right_op;
}

//-------------------------------------------------------------------------
// Name: ExecSubtract(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecSubtract(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecSubtract\n");
	return left_op - right_op;
}

//-------------------------------------------------------------------------
// Name: ExecMultiply(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecMultiply(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecMulti\n");
	return left_op * right_op;
}

//-------------------------------------------------------------------------
// Name: ExecMod(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecDivide(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecDivide\n");
	if (right_op == 0)
	{
		display_error = true;
		return 0L;
	}
	else
		return left_op / right_op;
}

//-------------------------------------------------------------------------
// Name: QExecMod(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecMod(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecMod\n");
	CALCAMNT temp = 0.0;

	if (right_op == 0)
	{
		display_error = true;
		return 0L;
	}
	else
	{
		// x mod y should be the same as x mod -y, thus:
		right_op = FABS(right_op);

		temp = FMOD(left_op, right_op);

		// let's make sure that -7 mod 3 = 2 and NOT -1.
		// In other words we wand x mod 3 to be a _positive_ number
		// that is 0,1 or 2.
		if( temp < 0 )
			temp = right_op + temp;

		return FABS(temp);
	}
}

//-------------------------------------------------------------------------
// Name: ExecIntDiv(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecIntDiv(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("IndDiv\n");
	if (right_op == 0)
	{
		display_error = true;
		return 0L;
	}
	else 
	{
		MODF(left_op / right_op, &left_op);
		return left_op;
	}
}

//-------------------------------------------------------------------------
// Name: ExecPower(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecPower(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecPowser %g left_op, %g right_op\n",left_op, right_op);
	if (right_op == 0)
		return 1L;
		
	if (left_op < 0 && isoddint(1 / right_op))
		left_op = -1L * POW((-1L * left_op), right_op);
	else
		left_op = POW(left_op, right_op);
		
	if (errno == EDOM || errno == ERANGE)
	{
		display_error = true;
		return 0;
	}
	else
		return left_op;
}

//-------------------------------------------------------------------------
// Name: ExecPwrRoot(CALCAMNT left_op, CALCAMNT right_op)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecPwrRoot(CALCAMNT left_op, CALCAMNT right_op)
{
	// printf("ExecPwrRoot  %g left_op, %g right_op\n", left_op, right_op);
	if (right_op == 0)
	{
		display_error = true;
		return 0L;
	}
	
	if (left_op < 0 && isoddint(right_op))
		left_op = -1L * POW((-1L * left_op), (1L)/right_op);
	else
		left_op = POW(left_op, (1L)/right_op);
		
	if (errno == EDOM || errno == ERANGE)
	{
		display_error = true;
		return 0;
	}
	else
		return left_op;
}

//-------------------------------------------------------------------------
// Name: ExecAddSubP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
//-------------------------------------------------------------------------	
CALCAMNT QtCalculator::ExecAddSubP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
{
	// printf("ExecAddsubP\n");
	UNUSED(left_op);

	if (result == 0)
	{
		display_error = true;
		return 0;
	} 
	else
		return (result * 100L) / right_op;
}

//-------------------------------------------------------------------------
// Name: ExecMultiplyP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecMultiplyP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
{
	// printf("ExecMultiplyP\n");
	UNUSED(left_op);
	UNUSED(right_op);
	return (result / 100L);
}

//-------------------------------------------------------------------------
// Name: ExecPowerP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecDivideP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
{
	// printf("ExecDivideP\n");
	UNUSED(left_op);
	UNUSED(right_op);
	return (result * 100L);
}

//-------------------------------------------------------------------------
// Name: ExecPowerP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecPowerP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
{
	// printf("ExecPowerP\n");
	UNUSED(result);
	return ExecPower(left_op, (right_op / 100L));
}

//-------------------------------------------------------------------------
// Name: ExecPwrRootP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
//-------------------------------------------------------------------------
CALCAMNT QtCalculator::ExecPwrRootP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result)
{
	// printf("ExePwrRootP\n");
	UNUSED(result);

	if (right_op == 0)
	{
		display_error = true;
		return 0;
	}
	else
		return ExecPower(left_op, (100L / right_op));
}


//-------------------------------------------------------------------------
// Name: AllocStackItem()
//-------------------------------------------------------------------------
stack_ptr AllocStackItem()
{
	if (stack_next <= stack_last)
	{
		process_stack[stack_next].prior_item = NULL;
		process_stack[stack_next].prior_type = NULL;
		return (process_stack + (stack_next++));
	}

	KMessageBox::error(0L, i18n("Stack Error!") );
	return (process_stack + stack_next);
}

//-------------------------------------------------------------------------
// Name: UnAllocStackItem(stack_ptr return_item)
//-------------------------------------------------------------------------
void UnAllocStackItem(stack_ptr return_item)
{
	if (return_item != (process_stack + (--stack_next)))
		KMessageBox::error(0L, i18n("Stack Error!") );
}

//-------------------------------------------------------------------------
// Name: PushStack(item_contents *add_item)
//-------------------------------------------------------------------------
void PushStack(item_contents *add_item)
{
	// Add an item to the stack

	stack_ptr new_item = top_of_stack;

	if (!(new_item && 
		new_item->item_value.s_item_type == add_item->s_item_type))
	{
		new_item = AllocStackItem();	// Get a new item

		// Chain new item to existing stacks


		new_item->prior_item = top_of_stack;
		top_of_stack	     = new_item;
		new_item->prior_type = top_type_stack[add_item->s_item_type];
		top_type_stack[add_item->s_item_type] = new_item;
	}

	new_item->item_value  = *add_item;	// assign contents

}

//-------------------------------------------------------------------------
// Name: PopStack()
//-------------------------------------------------------------------------
item_contents *PopStack()
{
	// Remove and return the top item in the stack

	static item_contents return_item;
	
	item_contents *return_item_ptr = NULL;
	stack_ptr return_stack_ptr;

	if ((return_stack_ptr = top_of_stack))
	{
		return_item = top_of_stack->item_value;

		top_type_stack[return_item.s_item_type] = 
			top_of_stack->prior_type;
	
		top_of_stack = top_of_stack->prior_item;

		UnAllocStackItem(return_stack_ptr);
	
		return_item_ptr = &return_item;
	}

	return return_item_ptr;
}

//-------------------------------------------------------------------------
// Name: TopTypeStack(item_type rqstd_type)
//-------------------------------------------------------------------------
item_contents *TopTypeStack(item_type rqstd_type)
{
	// Return the top item in the stack without removing

	item_contents *return_item_ptr = NULL;

	if (top_type_stack[rqstd_type])
		return_item_ptr = &(top_type_stack[rqstd_type]->item_value);

	return return_item_ptr;
}


/*
* Stack storage management Data and Functions
*/

//-------------------------------------------------------------------------
// Name: InitStack()
//-------------------------------------------------------------------------
void InitStack()
{
	stack_next = 0;
	stack_last = STACK_SIZE - 1;
	top_of_stack = top_type_stack[0] = top_type_stack[1] = NULL;
}
