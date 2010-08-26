/* This file is part of the KDE libraries
   Copyright (c) 2005 Klaus Niederkrueger <kniederk@math.uni-koeln.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "knumber_priv.h"

#include <math.h>
#include <config-kcalc.h>
#include <stdlib.h>
#ifdef Q_OS_LINUX
#include <signal.h>
#include <setjmp.h>
#endif

#include <QRegExp>

#if defined(Q_OS_SOLARIS) && defined(__SUNPRO_CC)
// Strictly by the standard, ininf() is a c99-ism which
// is unavailable in C++. The IEEE FP headers provide
// a function with similar functionality, so use that instead.
// However, !finite(a) == isinf(a) || isnan(a), so it's
// not 100% correct.
#include <ieeefp.h>
#define isinf(a) !finite(a)
#endif


detail::knumerror::knumerror(knumber const & num)
{
    switch (num.type()) {
    case SpecialType:
        error_ = dynamic_cast<knumerror const &>(num).error_;
        break;
    case IntegerType:
    case FractionType:
    case FloatType:
        // TODO: What should I do here?
        break;
    }
}

detail::knuminteger::knuminteger(qint64 num)
{
    mpz_init(mpz_);
#if SIZEOF_SIGNED_LONG == 8
    mpz_set_si(mpz_, static_cast<signed long int>(num));
#elif SIZEOF_SIGNED_LONG == 4
    mpz_set_si(mpz_, static_cast<signed long int>(num >> 32));
    mpz_mul_2exp(mpz_, mpz_, 32);
    mpz_add_ui(mpz_, mpz_, static_cast<signed long int>(num));
#else
#error "SIZEOF_SIGNED_LONG is a unhandled case"
#endif
}

detail::knuminteger::knuminteger(quint64 num)
{
    mpz_init(mpz_);
#if SIZEOF_UNSIGNED_LONG == 8
    mpz_set_ui(mpz_, static_cast<unsigned long int>(num));
#elif SIZEOF_UNSIGNED_LONG == 4
    mpz_set_ui(mpz_, static_cast<unsigned long int>(num >> 32));
    mpz_mul_2exp(mpz_, mpz_, 32);
    mpz_add_ui(mpz_, mpz_, static_cast<unsigned long int>(num));
#else
#error "SIZEOF_UNSIGNED_LONG is a unhandled case"
#endif
}

detail::knuminteger::knuminteger(knumber const & num)
{
    mpz_init(mpz_);

    switch (num.type()) {
    case IntegerType:
        mpz_set(mpz_, dynamic_cast<knuminteger const &>(num).mpz_);
        break;
    case FractionType:
    case FloatType:
    case SpecialType:
        // TODO: What should I do here?
        break;
    }
}

detail::knumfraction::knumfraction(knumber const & num)
{
    mpq_init(mpq_);

    switch (num.type()) {
    case IntegerType:
        mpq_set_z(mpq_, dynamic_cast<knuminteger const &>(num).mpz_);
        break;
    case FractionType:
        mpq_set(mpq_, dynamic_cast<knumfraction const &>(num).mpq_);
        break;
    case FloatType:
    case SpecialType:
        // What should I do here?
        break;
    }
}

detail::knumfloat::knumfloat(knumber const & num)
{
    mpf_init(mpf_);

    switch (num.type()) {
    case IntegerType:
        mpf_set_z(mpf_, dynamic_cast<knuminteger const &>(num).mpz_);
        break;
    case FractionType:
        mpf_set_q(mpf_, dynamic_cast<knumfraction const &>(num).mpq_);
        break;
    case FloatType:
        mpf_set(mpf_, dynamic_cast<knumfloat const &>(num).mpf_);
        break;
    case SpecialType:
        // What should I do here?
        break;
    }
}



detail::knumerror::knumerror(QString const & num)
{
    if (num == "nan")
        error_ = UndefinedNumber;
    else if (num == "inf")
        error_ = Infinity;
    else if (num == "-inf")
        error_ = MinusInfinity;
}

detail::knuminteger::knuminteger(QString const & num)
{
    mpz_init(mpz_);
    mpz_set_str(mpz_, num.toAscii(), 10);
}

detail::knumfraction::knumfraction(QString const & num)
{
    mpq_init(mpq_);
    if (QRegExp("^[+-]?\\d+(\\.\\d*)?(e[+-]?\\d+)?$").exactMatch(num)) {
        // my hand-made conversion is terrible
        // first me convert the mantissa
        unsigned long int digits_after_dot = ((num.section('.', 1, 1)).section('e', 0, 0)).length();
        QString tmp_num = num.section('e', 0, 0).remove('.');
        mpq_set_str(mpq_, tmp_num.toAscii(), 10);
        mpz_t tmp_int;
        mpz_init(tmp_int);
        mpz_ui_pow_ui(tmp_int, 10, digits_after_dot);
        mpz_mul(mpq_denref(mpq_), mpq_denref(mpq_), tmp_int);
        // now we take care of the exponent
        if (!(tmp_num = num.section('e', 1, 1)).isEmpty()) {
            long int tmp_exp = tmp_num.toLong();
            if (tmp_exp > 0) {
                mpz_ui_pow_ui(tmp_int, 10,
                              static_cast<unsigned long int>(tmp_exp));
                mpz_mul(mpq_numref(mpq_), mpq_numref(mpq_), tmp_int);
            } else {
                mpz_ui_pow_ui(tmp_int, 10,
                              static_cast<unsigned long int>(-tmp_exp));
                mpz_mul(mpq_denref(mpq_), mpq_denref(mpq_), tmp_int);
            }
        }
        mpz_clear(tmp_int);
    } else
        mpq_set_str(mpq_, num.toAscii(), 10);
    mpq_canonicalize(mpq_);
}

detail::knumfloat::knumfloat(QString const & num)
{
    mpf_init(mpf_);
    mpf_set_str(mpf_, num.toAscii(), 10);
}

detail::knuminteger const & detail::knuminteger::operator = (knuminteger const & num)
{
    if (this == &num)
        return *this;

    mpz_set(mpz_, num.mpz_);
    return *this;
}

QString const detail::knumerror::ascii(int prec) const
{
    Q_UNUSED(prec);

    // TODO: i18n these strings here and elsewhere, as they're user visible
    switch (error_) {
    case UndefinedNumber:
        return QString("nan");
    case Infinity:
        return QString("inf");
    case MinusInfinity:
        return QString("-inf");
    default:
        return QString();
    }
}

QString const detail::knuminteger::ascii(int prec) const
{
    Q_UNUSED(prec);
    char *tmp_ptr = 0;

    // get the size of the string
    size_t size = gmp_snprintf(tmp_ptr, 0, "%Zd", mpz_) + 1;
    tmp_ptr = new char[size];
    gmp_snprintf(tmp_ptr, size, "%Zd", mpz_);

    QString ret_str = tmp_ptr;

    delete [] tmp_ptr;
    return ret_str;
}

QString const detail::knumfraction::ascii(int prec) const
{
    Q_UNUSED(prec);
    char *tmp_ptr = mpq_get_str(0, 10, mpq_);
    QString ret_str = tmp_ptr;
    free(tmp_ptr);

    return ret_str;
}

QString const detail::knumfloat::ascii(int prec) const
{
    QString ret_str;
    char *tmp_ptr = 0;
    size_t size;
    if (prec > 0)
        size = gmp_snprintf(tmp_ptr, 0, ("%." + QString().setNum(prec) + "Fg").toAscii(), mpf_) + 1;
    else
        size = gmp_snprintf(tmp_ptr, 0, "%Fg", mpf_) + 1;
    tmp_ptr = new char[size];
    if (prec > 0)
        gmp_snprintf(tmp_ptr, size, ("%." + QString().setNum(prec) + "Fg").toAscii(), mpf_);
    else
        gmp_snprintf(tmp_ptr, size, "%Fg", mpf_);

    ret_str = tmp_ptr;

    delete [] tmp_ptr;

    return ret_str;
}


bool detail::knumfraction::isInteger(void) const
{
    if (mpz_cmp_ui(mpq_denref(mpq_), 1) == 0)
        return true;
    else
        return false;
}


detail::knumber * detail::knumerror::abs(void) const
{
    knumerror * tmp_num = new knumerror(*this);

    if (error_ == MinusInfinity) tmp_num->error_ = Infinity;

    return tmp_num;
}

detail::knumber * detail::knuminteger::abs(void) const
{
    knuminteger * tmp_num = new knuminteger();

    mpz_abs(tmp_num->mpz_, mpz_);

    return tmp_num;
}

detail::knumber * detail::knumfraction::abs(void) const
{
    knumfraction * tmp_num = new knumfraction();

    mpq_abs(tmp_num->mpq_, mpq_);

    return tmp_num;
}

detail::knumber * detail::knumfloat::abs(void) const
{
    detail::knumfloat * tmp_num = new detail::knumfloat();

    mpf_abs(tmp_num->mpf_, mpf_);

    return tmp_num;
}



detail::knumber * detail::knumerror::intPart(void) const
{
    return new knumerror(*this);
}

detail::knumber * detail::knuminteger::intPart(void) const
{
    detail::knuminteger *tmp_num = new detail::knuminteger();
    mpz_set(tmp_num->mpz_, mpz_);
    return tmp_num;
}

detail::knumber * detail::knumfraction::intPart(void) const
{
    detail::knuminteger *tmp_num = new detail::knuminteger();

    mpz_set_q(tmp_num->mpz_, mpq_);

    return tmp_num;
}

detail::knumber * detail::knumfloat::intPart(void) const
{
    detail::knuminteger *tmp_num = new detail::knuminteger();

    mpz_set_f(tmp_num->mpz_, mpf_);

    return tmp_num;
}




int detail::knumerror::sign(void) const
{
    switch (error_) {
    case Infinity:
        return 1;
    case MinusInfinity:
        return -1;
    default:
        return 0;
    }
}

int detail::knuminteger::sign(void) const
{
    return mpz_sgn(mpz_);
}

int detail::knumfraction::sign(void) const
{
    return mpq_sgn(mpq_);
}

int detail::knumfloat::sign(void) const
{
    return mpf_sgn(mpf_);
}



#ifdef __GNUC__
#warning "cube_root for now this is a stupid work around"
#endif

namespace {
void cube_root(mpf_t &num)
{
#ifdef Q_CC_MSVC
    double tmp_num = pow(mpf_get_d(num), 1. / 3.);
#else
    double tmp_num = ::cbrt(mpf_get_d(num));
#endif
    mpf_init_set_d(num, tmp_num);
}
}

detail::knumber * detail::knumerror::cbrt(void) const
{
    // infty ^3 = infty;  -infty^3 = -infty
    knumerror *tmp_num = new knumerror(*this);

    return tmp_num;
}

detail::knumber * detail::knuminteger::cbrt(void) const
{
    knuminteger * tmp_num = new knuminteger();

    if (mpz_root(tmp_num->mpz_, mpz_, 3))
        return tmp_num; // root is perfect

    delete tmp_num; // root was not perfect, result will be float

    knumfloat * tmp_num2 = new knumfloat();
    mpf_set_z(tmp_num2->mpf_, mpz_);

    cube_root(tmp_num2->mpf_);

    return tmp_num2;
}

detail::knumber * detail::knumfraction::cbrt(void) const
{
    knumfraction * tmp_num = new knumfraction();
    if (mpz_root(mpq_numref(tmp_num->mpq_), mpq_numref(mpq_), 3)
            &&  mpz_root(mpq_denref(tmp_num->mpq_), mpq_denref(mpq_), 3))
        return tmp_num; // root is perfect

    delete tmp_num; // root was not perfect, result will be float

    knumfloat * tmp_num2 = new knumfloat();
    mpf_set_q(tmp_num2->mpf_, mpq_);

    cube_root(tmp_num2->mpf_);

    return tmp_num2;
}

detail::knumber * detail::knumfloat::cbrt(void) const
{
    knumfloat * tmp_num = new knumfloat(*this);

    cube_root(tmp_num->mpf_);

    return tmp_num;
}




detail::knumber * detail::knumerror::sqrt(void) const
{
    knumerror *tmp_num = new knumerror(*this);

    if (error_ == MinusInfinity) tmp_num->error_ = UndefinedNumber;

    return tmp_num;
}

detail::knumber * detail::knuminteger::sqrt(void) const
{
    if (mpz_sgn(mpz_) < 0) {
        knumerror *tmp_num = new knumerror(UndefinedNumber);
        return tmp_num;
    }
    if (mpz_perfect_square_p(mpz_)) {
        knuminteger * tmp_num = new knuminteger();

        mpz_sqrt(tmp_num->mpz_, mpz_);

        return tmp_num;
    } else {
        knumfloat * tmp_num = new knumfloat();
        mpf_set_z(tmp_num->mpf_, mpz_);
        mpf_sqrt(tmp_num->mpf_, tmp_num->mpf_);

        return tmp_num;
    }
}

detail::knumber * detail::knumfraction::sqrt(void) const
{
    if (mpq_sgn(mpq_) < 0) {
        knumerror *tmp_num = new knumerror(UndefinedNumber);
        return tmp_num;
    }
    if (mpz_perfect_square_p(mpq_numref(mpq_))
            &&  mpz_perfect_square_p(mpq_denref(mpq_))) {
        knumfraction * tmp_num = new knumfraction();
        mpq_set(tmp_num->mpq_, mpq_);
        mpz_sqrt(mpq_numref(tmp_num->mpq_), mpq_numref(tmp_num->mpq_));
        mpz_sqrt(mpq_denref(tmp_num->mpq_), mpq_denref(tmp_num->mpq_));

        return tmp_num;
    } else {
        knumfloat * tmp_num = new knumfloat();
        mpf_set_q(tmp_num->mpf_, mpq_);
        mpf_sqrt(tmp_num->mpf_, tmp_num->mpf_);

        return tmp_num;
    }

    knumfraction * tmp_num = new knumfraction();

    return tmp_num;
}

detail::knumber * detail::knumfloat::sqrt(void) const
{
    if (mpf_sgn(mpf_) < 0) {
        knumerror *tmp_num = new knumerror(UndefinedNumber);
        return tmp_num;
    }
    knumfloat * tmp_num = new knumfloat();

    mpf_sqrt(tmp_num->mpf_, mpf_);

    return tmp_num;
}



detail::knumber * detail::knumerror::factorial(void) const
{
    return new knumerror(*this);
}

#ifdef Q_OS_LINUX
static jmp_buf abort_factorial;
static void factorial_abort_handler(int)
{
    longjmp(abort_factorial, 1);
}
#endif

detail::knumber * detail::knuminteger::factorial(void) const
{
    if (mpz_sgn(mpz_) < 0) {
        return new knumerror(UndefinedNumber);
    }

#ifdef Q_OS_LINUX
    struct sigaction new_sa;
    struct sigaction old_sa;

    memset(&new_sa, 0, sizeof(new_sa));

    new_sa.sa_handler = factorial_abort_handler;
    sigaction(SIGABRT, &new_sa, &old_sa);

    if(setjmp(abort_factorial)) {
		sigaction(SIGABRT, &old_sa, 0);
        return new knumerror(UndefinedNumber);
    }
#endif
    
    knuminteger * tmp_num = new knuminteger();
    mpz_fac_ui(tmp_num->mpz_, mpz_get_ui(mpz_));

#ifdef Q_OS_LINUX
    sigaction(SIGABRT, &old_sa, 0);
#endif

    return tmp_num;
}

detail::knumber * detail::knumfraction::factorial(void) const
{
    if (mpq_sgn(mpq_) < 0) {
        return new knumerror(UndefinedNumber);
    }
    knuminteger *tmp_num = new knuminteger();
    mpz_set_q(tmp_num->mpz_, mpq_);
    unsigned long int op = mpz_get_ui(tmp_num->mpz_);
	delete tmp_num;

    tmp_num = new knuminteger();

    mpz_fac_ui(tmp_num->mpz_, op);

    return tmp_num;
}

detail::knumber * detail::knumfloat::factorial(void) const
{
    if (mpf_sgn(mpf_) < 0) {
        return new knumerror(UndefinedNumber);
    }
    knuminteger *tmp_num = new knuminteger();
    mpz_set_f(tmp_num->mpz_, mpf_);
    unsigned long int op = mpz_get_ui(tmp_num->mpz_);
	delete tmp_num;

    tmp_num = new knuminteger();

    mpz_fac_ui(tmp_num->mpz_, op);

    return tmp_num;
}

detail::knumber * detail::knumerror::change_sign(void) const
{
    knumerror * tmp_num = new knumerror();

    if (error_ == Infinity) tmp_num->error_ = MinusInfinity;
    if (error_ == MinusInfinity) tmp_num->error_ = Infinity;

    return tmp_num;
}

detail::knumber * detail::knuminteger::change_sign(void) const
{
    knuminteger * tmp_num = new knuminteger();

    mpz_neg(tmp_num->mpz_, mpz_);

    return tmp_num;
}

detail::knumber * detail::knumfraction::change_sign(void) const
{
    knumfraction * tmp_num = new knumfraction();

    mpq_neg(tmp_num->mpq_, mpq_);

    return tmp_num;
}

detail::knumber *detail::knumfloat::change_sign(void) const
{
    knumfloat * tmp_num = new knumfloat();

    mpf_neg(tmp_num->mpf_, mpf_);

    return tmp_num;
}


detail::knumber * detail::knumerror::reciprocal(void) const
{
    switch (error_) {
    case  Infinity:
    case  MinusInfinity:
        return new knuminteger(0);
    case  UndefinedNumber:
    default:
        return new knumerror(UndefinedNumber);
    }
}

detail::knumber * detail::knuminteger::reciprocal(void) const
{
    if (mpz_cmp_si(mpz_, 0) == 0) return new knumerror(Infinity);

    knumfraction * tmp_num = new knumfraction(*this);

    mpq_inv(tmp_num->mpq_, tmp_num->mpq_);

    return tmp_num;
}

detail::knumber * detail::knumfraction::reciprocal() const
{
    if (mpq_cmp_si(mpq_, 0, 1) == 0) return new knumerror(Infinity);

    knumfraction * tmp_num = new knumfraction();

    mpq_inv(tmp_num->mpq_, mpq_);

    return tmp_num;
}

detail::knumber *detail::knumfloat::reciprocal(void) const
{
    if (mpf_cmp_si(mpf_, 0) == 0) return new knumerror(Infinity);

    knumfloat * tmp_num = new knumfloat();

    mpf_div(tmp_num->mpf_, knumfloat("1.0").mpf_, mpf_);

    return tmp_num;
}



detail::knumber * detail::knumerror::add(knumber const & arg2) const
{
    if (arg2.type() != SpecialType)
        return new knumerror(error_);

    knumerror const & tmp_arg2 = static_cast<knumerror const &>(arg2);

    if (error_ == UndefinedNumber
            || tmp_arg2.error_ == UndefinedNumber
            || (error_ == Infinity && tmp_arg2.error_ == MinusInfinity)
            || (error_ == MinusInfinity && tmp_arg2.error_ == Infinity)
       )
        return new knumerror(UndefinedNumber);

    return new knumerror(error_);
}

detail::knumber * detail::knuminteger::add(knumber const & arg2) const
{
    if (arg2.type() != IntegerType)
        return arg2.add(*this);

    knuminteger * tmp_num = new knuminteger();

    mpz_add(tmp_num->mpz_, mpz_,
            dynamic_cast<knuminteger const &>(arg2).mpz_);

    return tmp_num;
}

detail::knumber * detail::knumfraction::add(knumber const & arg2) const
{
    if (arg2.type() == IntegerType) {
        // need to cast arg2 to fraction
        knumfraction tmp_num(arg2);
        return tmp_num.add(*this);
    }


    if (arg2.type() == FloatType  ||  arg2.type() == SpecialType)
        return arg2.add(*this);

    knumfraction * tmp_num = new knumfraction();

    mpq_add(tmp_num->mpq_, mpq_,
            dynamic_cast<knumfraction const &>(arg2).mpq_);

    return tmp_num;
}

detail::knumber *detail::knumfloat::add(knumber const & arg2) const
{
    if (arg2.type() == SpecialType)
        return arg2.add(*this);

    if (arg2.type() != FloatType) {
        // need to cast arg2 to float
        knumfloat tmp_num(arg2);
        return tmp_num.add(*this);
    }

    knumfloat * tmp_num = new knumfloat();

    mpf_add(tmp_num->mpf_, mpf_,
            dynamic_cast<knumfloat const &>(arg2).mpf_);

    return tmp_num;
}


detail::knumber * detail::knumerror::multiply(knumber const & arg2) const
{
    //improve this
    switch (arg2.type()) {
    case SpecialType: {
        knumerror const & tmp_arg2 = static_cast<knumerror const &>(arg2);
        if (error_ == UndefinedNumber || tmp_arg2.error_ == UndefinedNumber)
            return new knumerror(UndefinedNumber);
        if (this->sign() * arg2.sign() > 0)
            return new knumerror(Infinity);
        else
            return new knumerror(MinusInfinity);
    }
    case IntegerType:
    case FractionType:
    case FloatType: {
        int sign_arg2 = arg2.sign();
        if (error_ == UndefinedNumber || sign_arg2 == 0)
            return new knumerror(UndefinedNumber);
        if ((error_ == Infinity  &&  sign_arg2 > 0)  ||
                (error_ == MinusInfinity  &&  sign_arg2 < 0))
            return new knumerror(Infinity);

        return new knumerror(MinusInfinity);
    }
    }

    return new knumerror(error_);
}


detail::knumber * detail::knuminteger::multiply(knumber const & arg2) const
{
    if (arg2.type() != IntegerType)
        return arg2.multiply(*this);

    knuminteger * tmp_num = new knuminteger();

    mpz_mul(tmp_num->mpz_, mpz_,
            dynamic_cast<knuminteger const &>(arg2).mpz_);

    return tmp_num;
}

detail::knumber * detail::knumfraction::multiply(knumber const & arg2) const
{
    if (arg2.type() == IntegerType) {
        // need to cast arg2 to fraction
        knumfraction tmp_num(arg2);
        return tmp_num.multiply(*this);
    }


    if (arg2.type() == FloatType  ||  arg2.type() == SpecialType)
        return arg2.multiply(*this);

    knumfraction * tmp_num = new knumfraction();

    mpq_mul(tmp_num->mpq_, mpq_,
            dynamic_cast<knumfraction const &>(arg2).mpq_);

    return tmp_num;
}

detail::knumber *detail::knumfloat::multiply(knumber const & arg2) const
{
    if (arg2.type() == SpecialType)
        return arg2.multiply(*this);
    if (arg2.type() == IntegerType  &&
            mpz_cmp_si(dynamic_cast<knuminteger const &>(arg2).mpz_, 0) == 0)
        // if arg2 == 0 return integer 0!!
        return new knuminteger(0);

    if (arg2.type() != FloatType) {
        // need to cast arg2 to float
        knumfloat tmp_num(arg2);
        return tmp_num.multiply(*this);
    }

    knumfloat * tmp_num = new knumfloat();

    mpf_mul(tmp_num->mpf_, mpf_,
            dynamic_cast<knumfloat const &>(arg2).mpf_);

    return tmp_num;
}





detail::knumber * detail::knumber::divide(knumber const & arg2) const
{
    knumber * tmp_num = arg2.reciprocal();
    knumber * rslt_num = this->multiply(*tmp_num);

    delete tmp_num;

    return rslt_num;
}

detail::knumber *detail::knumfloat::divide(knumber const & arg2) const
{
    if (mpf_cmp_si(mpf_, 0) == 0) return new knumerror(Infinity);

    // automatically casts arg2 to float
    knumfloat * tmp_num = new knumfloat(arg2);

    mpf_div(tmp_num->mpf_, mpf_, tmp_num->mpf_);

    return tmp_num;
}




detail::knumber * detail::knumerror::power(knumber const & /*exponent*/) const
{
    return new knumerror(UndefinedNumber);
}

detail::knumber * detail::knuminteger::power(knumber const & exponent) const
{
    if (exponent.type() == IntegerType) {

        mpz_t tmp_mpz;
        mpz_init_set(tmp_mpz,
                     dynamic_cast<knuminteger const &>(exponent).mpz_);

        if (! mpz_fits_ulong_p(tmp_mpz)) {     // conversion wouldn't work, so
            // use floats
            mpz_clear(tmp_mpz);
            // need to cast everything to float
            knumfloat tmp_num1(*this), tmp_num2(exponent);
            return tmp_num1.power(tmp_num2);
        }

        unsigned long int tmp_int = mpz_get_ui(tmp_mpz);
        mpz_clear(tmp_mpz);

        knuminteger * tmp_num = new knuminteger();
        mpz_pow_ui(tmp_num->mpz_, mpz_, tmp_int);
        return tmp_num;
    }
    if (exponent.type() == FractionType) {
        if (mpz_sgn(mpz_) < 0)
            return new knumerror(UndefinedNumber);
        // GMP only supports few root functions, so we need to convert
        // into signed long int
        mpz_t tmp_mpz;
        mpz_init_set(tmp_mpz,
                     mpq_denref(dynamic_cast<knumfraction const &>(exponent).mpq_));

        if (! mpz_fits_ulong_p(tmp_mpz)) {     // conversion wouldn't work, so
            // use floats
            mpz_clear(tmp_mpz);
            // need to cast everything to float
            knumfloat tmp_num1(*this), tmp_num2(exponent);
            return tmp_num1.power(tmp_num2);
        }

        unsigned long int tmp_int = mpz_get_ui(tmp_mpz);
        mpz_clear(tmp_mpz);

        // first check if result will be an integer
        knuminteger * tmp_num = new knuminteger();
        int flag = mpz_root(tmp_num->mpz_, mpz_, tmp_int);
        if (flag == 0) {   // result is not exact
            delete tmp_num;
            // need to cast everything to float
            knumfloat tmp_num1(*this), tmp_num2(exponent);
            return tmp_num1.power(tmp_num2);
        }

        // result is exact

        mpz_init_set(tmp_mpz,
                     mpq_numref(dynamic_cast<knumfraction const &>(exponent).mpq_));

        if (! mpz_fits_ulong_p(tmp_mpz)) {     // conversion wouldn't work, so
            // use floats
            mpz_clear(tmp_mpz);
            // need to cast everything to float
            knumfloat tmp_num1(*this), tmp_num2(exponent);
            return tmp_num1.power(tmp_num2);
        }
        tmp_int = mpz_get_ui(tmp_mpz);
        mpz_clear(tmp_mpz);

        mpz_pow_ui(tmp_num->mpz_, tmp_num->mpz_, tmp_int);

        return tmp_num;
    }
    if (exponent.type() == FloatType) {
        // need to cast everything to float
        knumfloat tmp_num(*this);
        return tmp_num.power(exponent);
    }

    return new knumerror(Infinity);
}

detail::knumber * detail::knumfraction::power(knumber const & exponent) const
{
    knuminteger tmp_num = knuminteger();

    mpz_set(tmp_num.mpz_, mpq_numref(mpq_));
    knumber *numer = tmp_num.power(exponent);

    mpz_set(tmp_num.mpz_, mpq_denref(mpq_));
    knumber *denom = tmp_num.power(exponent);

    knumber *result;

    if (numer->type() == SpecialType) {
        result = new knumerror(*numer);
    } else if (denom->type() == SpecialType) {
        result = new knumerror(*denom);
    } else {
        result = numer->divide(*denom);
    }

    delete numer;
    delete denom;
    return result;
}

detail::knumber * detail::knumfloat::power(knumber const & exponent) const
{
    double result = pow(static_cast<double>(*this),
                        static_cast<double>(exponent));
    if (isnan(result)) {
        return new knumerror(UndefinedNumber);
    }
    if (isinf(result)) {
        return new knumerror(Infinity);
    }

    return new knumfloat(result);
}


int detail::knumerror::compare(knumber const &arg2) const
{
    if (arg2.type() != SpecialType) {
        switch (error_) {
        case Infinity:
            return 1;
        case MinusInfinity:
            return -1;
        default:
            return 1; // Not really o.k., but what should I return
        }
    }

    switch (error_) {
    case Infinity:
        if (dynamic_cast<knumerror const &>(arg2).error_ == Infinity)
            // Infinity is larger than anything else, but itself
            return 0;
        return 1;
    case MinusInfinity:
        if (dynamic_cast<knumerror const &>(arg2).error_ == MinusInfinity)
            // MinusInfinity is smaller than anything else, but itself
            return 0;
        return -1;
    default:
        if (dynamic_cast<knumerror const &>(arg2).error_ == UndefinedNumber)
            // Undefined only equal to itself
            return 0;
        return -arg2.compare(*this);
    }
}

int detail::knuminteger::compare(knumber const &arg2) const
{
    if (arg2.type() != IntegerType)
        return - arg2.compare(*this);

    return mpz_cmp(mpz_, dynamic_cast<knuminteger const &>(arg2).mpz_);
}

int detail::knumfraction::compare(knumber const &arg2) const
{
    if (arg2.type() != FractionType) {
        if (arg2.type() == IntegerType) {
            mpq_t tmp_frac;
            mpq_init(tmp_frac);
            mpq_set_z(tmp_frac,
                      dynamic_cast<knuminteger const &>(arg2).mpz_);
            int cmp_result =  mpq_cmp(mpq_, tmp_frac);
            mpq_clear(tmp_frac);
            return cmp_result;
        } else
            return - arg2.compare(*this);
    }

    return mpq_cmp(mpq_, dynamic_cast<knumfraction const &>(arg2).mpq_);
}

int detail::knumfloat::compare(knumber const &arg2) const
{
    if (arg2.type() != FloatType) {
        mpf_t tmp_float;
        if (arg2.type() == IntegerType) {
            mpf_init(tmp_float);
            mpf_set_z(tmp_float,
                      dynamic_cast<knuminteger const &>(arg2).mpz_);
        } else if (arg2.type() == FractionType) {
            mpf_init(tmp_float);
            mpf_set_q(tmp_float,
                      dynamic_cast<knumfraction const &>(arg2).mpq_);
        } else
            return - arg2.compare(*this);

        int cmp_result =  mpf_cmp(mpf_, tmp_float);
        mpf_clear(tmp_float);
        return cmp_result;
    }

    return mpf_cmp(mpf_, dynamic_cast<knumfloat const &>(arg2).mpf_);
}



detail::knumerror::operator long int (void) const
{
    // what would be the correct return values here?
    if (error_ == Infinity)
        return 0;
    if (error_ == MinusInfinity)
        return 0;
    else // if (error_ == UndefinedNumber)
        return 0;
}

detail::knumerror::operator unsigned long int (void) const
{
    // what would be the correct return values here?
    if (error_ == Infinity)
        return 0;
    if (error_ == MinusInfinity)
        return 0;
    else // if (error_ == UndefinedNumber)
        return 0;
}

detail::knumerror::operator long long int (void) const
{
    // what would be the correct return values here?
    if (error_ == Infinity)
        return 0;
    if (error_ == MinusInfinity)
        return 0;
    else // if (error_ == UndefinedNumber)
        return 0;
}

detail::knumerror::operator unsigned long long int (void) const
{
    // what would be the correct return values here?
    if (error_ == Infinity)
        return 0;
    if (error_ == MinusInfinity)
        return 0;
    else // if (error_ == UndefinedNumber)
        return 0;
}

detail::knuminteger::operator long int (void) const
{
    return mpz_get_si(mpz_);
}

detail::knumfraction::operator long int (void) const
{
    return static_cast<long int>(mpq_get_d(mpq_));
}

detail::knumfloat::operator long int (void) const
{
    return mpf_get_si(mpf_);
}

detail::knuminteger::operator unsigned long int (void) const
{
    return mpz_get_ui(mpz_);
}

detail::knuminteger::operator long long int (void) const
{
    // libgmp doesn't have long long conversion
    // so convert to string and then to long long
    char *tmpchar = new char[mpz_sizeinbase(mpz_, 10) + 2];
    mpz_get_str(tmpchar, 10, mpz_);
    QString tmpstring(tmpchar);
    delete [] tmpchar;
    bool ok;
    long long int value = tmpstring.toLongLong(&ok, 10);

    if (!ok) {
        // TODO: what to do if error?
        value = 0;
    }
    return value;
}

detail::knuminteger::operator unsigned long long int (void) const
{
    // libgmp doesn't have unsigned long long conversion
    // so convert to string and then to unsigned long long
    char *tmpchar = new char[mpz_sizeinbase(mpz_, 10) + 2];
    mpz_get_str(tmpchar, 10, mpz_);
    QString tmpstring(tmpchar);
    delete [] tmpchar;

    bool ok;
    unsigned long long int value;
    if (sign() < 0) {
        long long signedvalue = tmpstring.toLongLong(&ok, 10);
        value = static_cast<unsigned long long>(signedvalue);
    } else {
        value = tmpstring.toULongLong(&ok, 10);
    }

    if (!ok) {
        // TODO: what to do if error?
        value = 0;
    }
    return value;
}

detail::knumfraction::operator unsigned long int (void) const
{
    return static_cast<unsigned long int>(mpq_get_d(mpq_));
}

detail::knumfraction::operator long long int (void) const
{
    return static_cast<long long int>(mpq_get_d(mpq_));
}

detail::knumfraction::operator unsigned long long int (void) const
{
    return static_cast<unsigned long long int>(mpq_get_d(mpq_));
}

detail::knumfloat::operator unsigned long int (void) const
{
    return mpf_get_ui(mpf_);
}

detail::knumfloat::operator long long int (void) const
{
    return static_cast<long long int>(mpf_get_si(mpf_));
}

detail::knumfloat::operator unsigned long long int (void) const
{
    return static_cast<unsigned long long int>(mpf_get_ui(mpf_));
}

detail::knumerror::operator double(void) const
{
    if (error_ == Infinity)
        return INFINITY;
    if (error_ == MinusInfinity)
        return -INFINITY;
    else // if (error_ == UndefinedNumber)
        return NAN;
}

detail::knuminteger::operator double(void) const
{
    return mpz_get_d(mpz_);
}

detail::knumfraction::operator double(void) const
{
    return mpq_get_d(mpq_);
}

detail::knumfloat::operator double(void) const
{
    return mpf_get_d(mpf_);
}




detail::knuminteger * detail::knuminteger::intAnd(knuminteger const &arg2) const
{
    knuminteger * tmp_num = new knuminteger();

    mpz_and(tmp_num->mpz_, mpz_, arg2.mpz_);

    return tmp_num;
}

detail::knuminteger * detail::knuminteger::intOr(knuminteger const &arg2) const
{
    knuminteger * tmp_num = new knuminteger();

    mpz_ior(tmp_num->mpz_, mpz_, arg2.mpz_);

    return tmp_num;
}

detail::knumber * detail::knuminteger::mod(knuminteger const &arg2) const
{
    if (mpz_cmp_si(arg2.mpz_, 0) == 0) return new knumerror(UndefinedNumber);

    knuminteger * tmp_num = new knuminteger();

    mpz_mod(tmp_num->mpz_, mpz_, arg2.mpz_);

    return tmp_num;
}

detail::knumber * detail::knuminteger::shift(knuminteger const &arg2) const
{
    mpz_t tmp_mpz;

    mpz_init_set(tmp_mpz, arg2.mpz_);

    if (! mpz_fits_slong_p(tmp_mpz)) {
        mpz_clear(tmp_mpz);
        return new knumerror(UndefinedNumber);
    }

    signed long int tmp_arg2 = mpz_get_si(tmp_mpz);
    mpz_clear(tmp_mpz);


    knuminteger * tmp_num = new knuminteger();

    if (tmp_arg2 > 0)    // left shift
        mpz_mul_2exp(tmp_num->mpz_, mpz_, tmp_arg2);
    else {
        // right shift
        if(mpz_sgn(mpz_) < 0) {
            mpz_fdiv_q_2exp(tmp_num->mpz_, mpz_, -tmp_arg2);
        } else {
            mpz_tdiv_q_2exp(tmp_num->mpz_, mpz_, -tmp_arg2);
        }
    }

    return tmp_num;
}

