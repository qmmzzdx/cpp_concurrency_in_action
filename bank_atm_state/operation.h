#ifndef OPERATION_H
#define OPERATION_H

#include <string>
#include "messaging.h"

struct withdraw
{
    std::string account;
    unsigned amount;
    mutable messaging::sender atm_queue;
    withdraw(const std::string &_act, unsigned _amt, messaging::sender _atq) : account(_act), amount(_amt), atm_queue(_atq)
    {
    }
};

struct withdraw_ok
{
};

struct withdraw_denied
{
};

struct cancel_withdrawal
{
    std::string account;
    unsigned amount;
    cancel_withdrawal(const std::string &_act, unsigned _amt) : account(_act), amount(_amt)
    {
    }
};

struct withdrawal_processed
{
    std::string account;
    unsigned amount;
    withdrawal_processed(const std::string &_act, unsigned _amt) : account(_act), amount(_amt)
    {
    }
};

struct card_inserted
{
    std::string account;
    explicit card_inserted(const std::string &_act) : account(_act)
    {
    }
};

struct digit_pressed
{
    char digit;
    explicit digit_pressed(char _digit) : digit(_digit)
    {
    }
};

struct clear_last_pressed
{
};

struct eject_card
{
};

struct withdraw_pressed
{
    unsigned amount;
    explicit withdraw_pressed(unsigned _amt) : amount(_amt)
    {
    }
};

struct cancel_pressed
{
};

struct issue_money
{
    unsigned amount;
    issue_money(unsigned _amt) : amount(_amt)
    {
    }
};

struct verify_pin
{
    std::string account;
    std::string pin;
    mutable messaging::sender atm_queue;
    verify_pin(const std::string &_act, const std::string &_pin, messaging::sender _atq) : account(_act), pin(_pin), atm_queue(_atq)
    {
    }
};

struct pin_verified
{
};

struct pin_incorrect
{
};

struct display_enter_pin
{
};

struct display_enter_card
{
};

struct display_insufficient_funds
{
};

struct display_withdrawal_cancelled
{
};

struct display_pin_incorrect_message
{
};

struct display_withdrawal_options
{
};

struct get_balance
{
    std::string account;
    mutable messaging::sender atm_queue;
    get_balance(const std::string &_act, messaging::sender _atq) : account(_act), atm_queue(_atq)
    {
    }
};

struct balance
{
    unsigned amount;
    explicit balance(unsigned _amt) : amount(_amt)
    {
    }
};

struct display_balance
{
    unsigned amount;
    explicit display_balance(unsigned _amt) : amount(_amt)
    {
    }
};

struct balance_pressed
{
};

#endif
