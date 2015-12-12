/*
 * RPGServ wod functions
 */

#include "module.h"

class CommandRSExalted : public Command
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
	CommandRSExalted(Module *creator) : Command(creator, "rpgserv/exalted", 2, 4)
	{
		this->SetDesc(_("Rolls dice using the Exalted dice system.")) ;
		this->SetSyntax(_("\002\037<#dice>\037 \037<Y/N>\037 \037#channel\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		//************************************
		//
		//************************************
		const Anope::string &dice = params[0];
		const Anope::string &flags = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		int diff = 7;
		int i = 0;
		int success=0;
		Anope::string target = "";
		Anope::string results = "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
	  srand(time(NULL));
		if(!dice.is_number_only() || dice[0]=='-' || dice[0]=='+' || flags.find_first_of("ynYN") == Anope::string::npos) {
			this->SendSyntax(source);
			return;
		}
		int many = convertTo<int>(dice.c_str());
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
					target = u->nick.c_str();
				}
			}
		}

		if ( convertTo<int>(dice.c_str()) > dicemax)
		{
			source.Reply(_("Maximum number of %d dice exceeded."),dicemax);
			return;
		}
		for(i=many; i>0; i--)
		{
			int q;
			q=roll(1,10);
			if(q >= diff) 
			{
				success++;
			}
			if(q == 10 && flags.find_first_of("y"))
			{
				success++;
			}
			results.append(" ");
			results.append(stringify(q));
		}
		//Output is "Total Dice %s, Diff was %d: Successes <%d>, Total <%d>."
		Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << flags << " " << chan << " " << message;
		if (!target.empty())
		{
			if (!u2)
			{
			   IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled %d dice, Diff was %d: %s", u->nick.c_str(), many, diff, message.c_str());
			   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Rolls: %s ", results.c_str());
			   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Successes: %d ", success);
         }
         else 
         {
			   u2->SendMessage(bi, _("%s Rolled %d dice, Diff was %d: %s"), u->nick.c_str(), many, diff, message.c_str());
			   u2->SendMessage(bi, _("|-- Rolls: %s "), results.c_str());
			   u2->SendMessage(bi, _("|-- Successes: %d "), success);
         }
			source.Reply("Rolled %d dice, Diff was %d: %s", many, diff, message.c_str());
			source.Reply("|-- Rolls: %s ", results.c_str());
			source.Reply("+-- Successes: %d ", success);
		}
		else
		{
			u->SendMessage(bi, _("Rolled %d dice, Diff was %d: %s"), many, diff, message.c_str());
			u->SendMessage(bi, _("|-- Roll: %s "), results.c_str());
			u->SendMessage(bi, _("+-- Successes: %d"), success);
		}
		return;
	}
	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Roll Exalted system dice.\n"
			" Help not currently available\n"
			" \n"));
		return true;
	}
};

class RSExalted : public Module
{
	CommandRSExalted commandrsexalted;

 public:
	RSExalted(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrsexalted(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSExalted)
