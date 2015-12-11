/*
 * RPGServ wod functions
 */

#include "module.h"

class CommandRSd20 : public Command
{
 private:

	int ran(int k) {
		double x = RAND_MAX + 1.0;
		int y;
		y = 1 + rand() * (k / x);
		return ( y );
	}

	int roll(int many, int sides) {
		int i,q=0;
		if(many>10000)
			many=10000;
		if(sides>1000000)
			sides=1000000;
		for (i=0;i<many;i++) {
			q += ran(sides);
		}
		return ( q );
	}

 public:
	CommandRSd20(Module *creator) : Command(creator, "rpgserv/d20", 2, 4)
	{
		this->SetDesc(_("Rolls a d20 die against a difficilty.")) ;
		this->SetSyntax(_("\002\037<diff>\037 \037<+/-modifiers>\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		//************************************
		//
		//************************************
		const Anope::string &difficulty = params[0];
		const Anope::string &modifiers = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		Anope::string target = "";
		int i = 0;
		int result = 0;
		int modd= 0;
		int diff= 0 ;
		int success=0;
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
		if(!difficulty.is_pos_number_only()) {
			this->SendSyntax(source);
			return;
		}
		Anope::string modrest = modifiers.substr(1,modifiers.length()) ;
		if ( (modifiers[0]=='+' || modifiers[0]=='-') && modrest.is_number_only() )
		{
			diff = convertTo<int>(difficulty);
			modd = convertTo<int>(modifiers);
			result = roll(1,20);
			i = result + modd;
			if(i>=diff) 
				success=1;
			else
				success=0;
		}
		else 
		{
			source.Reply(_("Invalid syntax."));
			this->SendSyntax(source);
			return;
		}

		if(!chan.empty()) 
		{
			if (chan[0] == '#')
			{
				if (!IRCD->IsChannelValid(chan))
					source.Reply(CHAN_X_INVALID, chan.c_str());
				else
				{
					ChannelInfo *ci = ChannelInfo::Find(chan);
					if (!ci && u)
					{
						for (channel_map::const_iterator cit = ChannelList.begin(), cit_end = ChannelList.end(); cit != cit_end; ++cit)
						{
							Channel *c = cit->second;
							if (!Anope::Match(c->name, chan, false, true))
								continue;
							target = chan.c_str();
						}
						if (target.empty()) {
							source.Reply(CHAN_X_NOT_IN_USE, chan.c_str());
							return;
						}
					}
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
		Log(LOG_COMMAND,source, this) << "to roll a d20 with a modifier " << modd << " with a message of:" << message.c_str() 
			<< " to " << target.c_str();
		if (!target.empty())
		{
			if (!u2)
			{
  			   IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled a d20: %s", u->nick.c_str(), message.c_str());
			   IRCD->SendPrivmsg(bi, target.c_str(), "|-- roll of %d with a modifier of %d results in %d", result, modd, i);
			   if(success==1)
			   {
			  	   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Check against difficulty of %s was successful.", difficulty.c_str());
            }
            else
            {
				   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Check against difficulty of %s failed.", difficulty.c_str());
            }
         }
         else
         {
			   u2->SendMessage(bi, _("%s Rolled a d20: %s"), u->nick.c_str(), message.c_str());
			   u2->SendMessage(bi, _("|-- roll of %d with a modifier of %d results in %d"), result, modd, i);
			   if(success==1)
			   {
				   u2->SendMessage(bi, _("+-- Check against difficulty of %s was successful."), difficulty.c_str());
            }
            else
            {
				   u2->SendMessage(bi, _("+-- Check against difficulty of %s failed."), difficulty.c_str());
            }
         }
			source.Reply("Rolled a d20: %s", message.c_str());
			source.Reply("|-- roll of %d with a modifier of %d results in %d", result, modd, i);
			if(success==1)
			{
				source.Reply("+-- Check against difficulty of %s was successful.", difficulty.c_str());
			}
			else
			{
				source.Reply("+-- Check against difficulty of %s failed..", difficulty.c_str());
			}
		}
		else
		{
			u->SendMessage(bi, _("%s Rolled a d20: %s"), u->nick.c_str(), message.c_str());
			u->SendMessage(bi, _("|-- roll of %d with a modifier of %d results in %d"), result, modd, i);
			if(success==1)
			{
				u->SendMessage(bi, _("+-- Check against difficulty of %s was successful."), difficulty.c_str());
			}
			else
			{
				u->SendMessage(bi, _("+-- Check against difficulty of %s failed."), difficulty.c_str());
			}
		}
		return;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Roll d20 Style dice.\n"
			"This roll is used to simulate the d20 checks from the new d20 gaming\n"
			"systems.\n"
			" \n"
			"The +/- part of the modifier is not optional.\n"
			" \n"
			"If the DM has not made the difficulty known, a +0 can be entered and the\n"
			"command will still return the result of the d20 roll.  For example, if\n"
			"Bob does not know his DC, he can still /msg RPGServ d20 4 +0, and the\n"
			"result of the roll will be returned without a pass/fail statement.\n"
			" \n"
			"Note, that as with all RPGServ commands, to use the descriptive text you\n"
			"must supply the where value for the secondary recipient of the results\n"
			"(be that a channel or a person).\n"
			" \n"));
		return true;
	}
};

class RSd20 : public Module
{
	CommandRSd20 commandrsd20;

 public:
	RSd20(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrsd20(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSd20)
