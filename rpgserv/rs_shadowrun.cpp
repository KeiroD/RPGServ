/*
 * RPGServ shadowrun functions
 */

#include "module.h"

class CommandRSShadowrun : public Command
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
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		int sidemax = Config->GetModule("rpgserv")->Get<int>("sidemax","100");
		if(many>dicemax)
			many=dicemax;
		if(sides>sidemax)
			sides=sidemax;
		for (i=0;i<many;i++) {
			q += ran(sides);
		}
		return ( q );
	}

 public:
	CommandRSShadowrun(Module *creator) : Command(creator, "rpgserv/shadowrun", 2, 4)
	{
		this->SetDesc(_("Rolls dice using the Shadowrun dice system.")) ;
		this->SetSyntax(_("\002\037<dice>\037 \037difficulty\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		//Syntax: Shadowrun <how many> <difficulty> [optional where|who] 
		const Anope::string &dice = params[0];
		const Anope::string &difficulty = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		Anope::string target = "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
      User *u2 = NULL;
		User *u = source.GetUser();
		if(!dice.is_number_only() || !difficulty.is_number_only()) {
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
            Log() << "Looking for '" << chan << "'";
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

		//need to parse the dice, but first lets catch for fun items
		if ( convertTo<int>(dice.c_str()) > dicemax )
		{
			source.Reply(_("Maximum number of %d dice exceeded."),dicemax);
			return;
		}
		else if(!dice.empty())
		{
			//Process dice normally
			int many = convertTo<int>(dice);
			int diff = convertTo<int>(difficulty);
			int i;
			int r;
			Anope::string results;
			Anope::string myroll2;
			int successes = 0;
			int myroll;
			if ( many < 1 || diff < 1 )
			{
				source.Reply(_("you must roll at least 1 die, and have a difficulty of at least 1.\n"));
			}
                        for(i = many; i > 0 ; i--)
			{
				r = 0;
				myroll = roll(1,6);
				if(myroll >= diff)
					successes++;
				while(myroll==6)
				{
					r = r + myroll;
					myroll=roll(1,6);
					if(myroll >= diff)
						successes++;
				}
				r = r + myroll;
			        results.append(" ");
			        myroll2 = stringify(r);
				results.append(myroll2);
			}
			Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << difficulty << " " << chan << " " << message;
			if (!target.empty())
			{
			   if (!u2)
			   {
				  IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled against a difficulty of %d:  %s", u->nick.c_str(), diff, message.c_str());
				  IRCD->SendPrivmsg(bi, target.c_str(), "|-- Dice Rolled: %s ", results.c_str());
				  IRCD->SendPrivmsg(bi, target.c_str(), "+-- Successes: %d ", successes);
            }
            else
            {
				  u2->SendMessage(bi, _("%s Rolled against a difficulty of %d:  %s"), u->nick.c_str(), diff, message.c_str());
				  u2->SendMessage(bi, _("|-- Dice Rolled: %s "), results.c_str());
				  u2->SendMessage(bi, _("+-- Successes: %d "), successes);
            }
				source.Reply("Rolled against a difficulty of %d:  %s", diff, message.c_str());
				source.Reply("|-- Dice Rolled: %s ",  results.c_str());
				source.Reply("+-- Successes: %d ", successes);
			}
			else
			{
				u->SendMessage(bi, _("%s Rolled against a difficulty of %d: %s"), u->nick.c_str(), diff, message.c_str());
				u->SendMessage(bi, _("|-- Rolled: %s "), results.c_str());
				u->SendMessage(bi, _("+-- Successes: %d"), successes);
			}
			return;
		} 
		else 
		{
			this->SendSyntax(source);
			return;
		}
		return;
	}
	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Roll Shadowrun dice.\n"
			"Help not currently available \n"
			" \n"));
		return true;
	}
};

class RSShadowrun : public Module
{
	CommandRSShadowrun commandrsshadowrun;

 public:
	RSShadowrun(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrsshadowrun(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSShadowrun)
