#pragma once
#include "common-pragmas.h"

#include "common.h"


void init_coinNetwork(std::shared_ptr<t_coin_info> coin);

bool pollLocal(std::shared_ptr<t_coin_info> coinInfo);

void hostname_to_ip(char const *const  in_addr, char* out_addr);
void updater_i(std::shared_ptr<t_coin_info> coinInfo);
void proxy_i(std::shared_ptr<t_coin_info> coinInfo);
void send_i(std::shared_ptr<t_coin_info> coinInfo);
void confirm_i(std::shared_ptr<t_coin_info> coinInfo);
size_t xstr2strr(char *buf, size_t const bufsize, const char *const in);
