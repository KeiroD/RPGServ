/*
 * RPGServ wod functions
 */

#include "module.h"

class CommandRSWw : public Command
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
	CommandRSWw(Module *creator) : Command(creator, "rpgserv/ww", 2, 4)
	{
		this->SetDesc(_("Rolls dice using the Newer World of Darkness dice system.")) ;
		this->SetSyntax(_("\002\037<#dice>\037 \037Chance (Y/N)\037 \037#channel\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		//************************************
		// if flag is set, for chance roll
		//   number of dice is limitied to 1
		//   Diff is 10. Can have Dramatic
		//   failure.
		//
		// if flag is not set, regular roll
		//   number of dice is limited to 30
		//   Diff is 8. Can not have Dramatic
		//   failure.
		//
		//************************************
		const Anope::string &dice = params[0];
		const Anope::string &flags = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		int diff;
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");

		Anope::string target = "";
		if(flags.equals_ci("y"))
			diff = 10;
		else
			diff = 8;
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
	  srand(time(NULL));
		if(!dice.is_number_only()) 
		{
			this->SendSyntax(source);
			return;
		}
		else if ( !dice.is_pos_number_only() )
		{
			source.Reply(_("Number of dice must be greater than 0."));
			return;
		}
		else if ( convertTo<int>(dice.c_str()) > dicemax )
		{
			source.Reply(_("Number of dice cannot exceede %s"),dicemax);
			return;
		}
		if ( flags.find_first_of("ynYN") == Anope::string::npos )
		{
			source.Reply(_("You must specify a Y or N flag."));
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
					target = u->nick.c_str();
				}
			}
		}

		/* need to parse the dice, but first lets catch for fun items  */
		if (!flags.equals_ci("y") && convertTo<int>(dice.c_str())>dicemax)
		{
			source.Reply(_("Maximum number of %d dice exceeded."),dicemax);
			return;
		}
		else if(flags.equals_ci("y") && convertTo<int>(dice.c_str()) >1)
		{
			source.Reply("Maximum number of 1 die exceeded.");
			return;
		}
		else if(!dice.empty())
		{
			//Process dice normally
			int many = convertTo<int>(dice);
			int i;
			Anope::string results;
			Anope::string myroll2;
			int successes = 0;
			int botches = 0;
			int total = 0;
			int myroll;
                        for(i = 0; i < many; i++)
			{
				myroll = roll(1,10);
				if(i>0)
					results.append(" ");
			        myroll2 = stringify(myroll);
				results.append(myroll2);
				if(myroll == 1)
				{
					botches++;
					total--;
				}
				else if(myroll >= diff)
				{
					successes++;
					total++;
				}
			}
			Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << flags << " " << chan << " " << message;
			if (!target.empty())
			{
			   if (!u2)
			   {
     				IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled %d dice: %s", u->nick.c_str(), many, message.c_str());
				   if ( total < 0 && flags.equals_ci("y") )
				   {
					   IRCD->SendPrivmsg(bi, target.c_str(), "|-- DRAMATIC FAILURE ");
				   }
				   if ( flags.equals_ci("n") )
			   	{
				   	IRCD->SendPrivmsg(bi, target.c_str(), "|-- Total: %d ", total);
				   }
				   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Dice Rolled: %s ", results.c_str());
				   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Successes: %d", successes);
            }
            else
            {
     				u2->SendMessage(bi, _("%s Rolled %d dice: %s"), u->nick.c_str(), many, message.c_str());
				   if ( total < 0 && flags.equals_ci("y") )
				   {
					   u2->SendMessage(bi, _("|-- DRAMATIC FAILURE "));
				   }
				   if ( flags.equals_ci("n") )
				   {
					   u2->SendMessage(bi, _("|-- Total: %d "), total);
				   }
				   u2->SendMessage(bi, _("|-- Dice Rolled: %s "), results.c_str());
				   u2->SendMessage(bi, _("+-- Successes: %d"), successes);
            }
				source.Reply("Rolled %d dice: %s", many, message.c_str());
				if ( total < 0 && flags.equals_ci("y") )
				{
					source.Reply("|-- DRAMATIC FAILURE");
				}
				if ( flags.equals_ci("n") )
				{
					source.Reply("|-- Total: %d ", total);
				}
				source.Reply("|-- Dice Rolled: %s ", results.c_str());
				source.Reply("+-- Successes: %d", successes);
			}
			else
			{
				IRCD->SendPrivmsg(bi, u->nick.c_str(), "Rolled %d dice: %s", many, message.c_str());
				if(total < 0 && flags.equals_ci("y"))
					IRCD->SendPrivmsg(bi, u->nick.c_str(), "|-- DRAMATIC FAILURE");
				IRCD->SendPrivmsg(bi, u->nick.c_str(), "|-- Rolled: %s ", results.c_str());
				IRCD->SendPrivmsg(bi, u->nick.c_str(), "+-- Successes: %d", botches);
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
		source.Reply(_("Roll Newer Style World of darkness dice.\n"
			"This command is to simulate the rolling of dice for the\n"
			"NEW World Of Darkness dice system. Dice are rolled against\n"
			"the target numbner 8 or 10 for chance rolls. \n"
			" \n"
			"The chance flag specifies if the roll is to be calculated as a chance roll\n"
			"if the roll is calculated as a chance roll Dramatic Failure is calculated\n"
			" \n"
			"The [optional where] paramater is as implied an\n"
			"optional paramater. It specifies a location in addition\n"
			"to the user to which output is sent. This can be either\n"
			"a channel or another username. No error checking is\n"
			"performed on this parameter and if invalid information\n"
			"is supplied it will simply output to the user alone.\n"
			" \n"
			"Note, that if the chance information is ommited or replaced with anything other\n"
			"than Y the value is assumed to be No. However if you wish to send to a destination\n"
			"channel or person you MUST specify the chance value\n"
			" \n"));
		return true;
	}
};

class RSWw : public Module
{
	CommandRSWw commandrswod;

 public:
	RSWw(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrswod(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSWw)
