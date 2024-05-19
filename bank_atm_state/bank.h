#ifndef BANKMACHINE_H
#define BANKMACHINE_H

#include "operation.h"

class bank_machine
{
private:
    messaging::receiver incoming;
    unsigned balance;

public:
    bank_machine() : balance(199)
    {
    }
    void done()
    {
        get_sender().send(messaging::close_queue());
    }
    void run()
    {
        try
        {
            for (;;)
            {
                incoming.wait()
                    .handle<verify_pin>(
                        [&](const verify_pin &msg)
                        {
                            if (msg.pin == "1937")
                            {
                                msg.atm_queue.send(pin_verified());
                            }
                            else
                            {
                                msg.atm_queue.send(pin_incorrect());
                            }
                        })
                    .handle<withdraw>(
                        [&](const withdraw &msg)
                        {
                            if (balance >= msg.amount)
                            {
                                msg.atm_queue.send(withdraw_ok());
                                balance -= msg.amount;
                            }
                            else
                            {
                                msg.atm_queue.send(withdraw_denied());
                            }
                        })
                    .handle<get_balance>(
                        [&](const get_balance &msg)
                        {
                            msg.atm_queue.send(::balance(balance));
                        })
                    .handle<withdrawal_processed>(
                        [&](const withdrawal_processed &msg) {
                        })
                    .handle<cancel_withdrawal>(
                        [&](const cancel_withdrawal &msg) {
                        });
            }
        }
        catch (const messaging::close_queue &)
        {
        }
    }
    messaging::sender get_sender()
    {
        return incoming;
    }
};

#endif
