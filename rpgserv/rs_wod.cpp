/*
 * RPGServ wod functions
 */

#include "module.h"

class CommandRSWod : public Command
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
	CommandRSWod(Module *creator) : Command(creator, "rpgserv/wod", 2, 4)
	{
		this->SetDesc(_("Rolls dice using the Old World of Darkness dice system.")) ;
		this->SetSyntax(_("\002\037<dice>\037 \037difficulty\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &dice = params[0];
		const Anope::string &difficulty = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		Anope::string target = "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      User *u2 = NULL;
		if(!dice.is_number_only() || !difficulty.is_number_only()) 
		{
			this->SendSyntax(source);
			return;
		}
		else if(!dice.is_pos_number_only() || !difficulty.is_pos_number_only() )
		{
			source.Reply(_("Only positive values allowed for number of dice and difficulty."));
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

		//need to parse the dice, but first lets catch for fun items
		if ( convertTo<int>(dice.c_str()) > dicemax)
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
			Anope::string results;
			Anope::string myroll2;
			int successes = 0;
			int botches = 0;
			int total = 0;
			int specialization = 0;
			int myroll;
			if (diff > 10)
			{
				source.Reply(_("You cannot have a difficulty greater than 10."));
				return;
			}
                        for(i = 0; i < many; i++)
			{
				myroll = roll(1,10);
				if(i>0)
					results.append(" ");
			        myroll2 = stringify(myroll);
				results.append(myroll2);
				if(myroll == 10)
					specialization++;
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
			Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << difficulty << " " << chan << " " << message;
			if (!target.empty())
			{
			   if (!u2)
			   {
				   IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled %d dice with a difficulty of %d:  %s", u->nick.c_str(), many, diff, message.c_str());
				   if(total < 0){
					   IRCD->SendPrivmsg(bi, target.c_str(), "|-- * * * BOTCHED * * * ");
               }
				   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Total: %d ", total);
				   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Dice Rolled: %s ", results.c_str());
				   if ( botches > 0 ) {
					   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Total Botches: %d", botches);
               }
				   if ( specialization > 0 ) 
				   {
					   IRCD->SendPrivmsg(bi, target.c_str(), "|-- Specialization rerolls: %d", specialization);
               }
				   IRCD->SendPrivmsg(bi, target.c_str(), "+-- Total Successes: %d", successes);
            }
            else
            {
				   u2->SendMessage(bi, _("%s Rolled %d dice with a difficulty of %d:  %s"), u->nick.c_str(), many, diff, message.c_str());
				   if(total < 0){
					   u2->SendMessage(bi, _("|-- * * * BOTCHED * * * "));
               }
				   u2->SendMessage(bi, _("|-- Total: %d "), total);
				   u2->SendMessage(bi, _("|-- Dice Rolled: %s "), results.c_str());
				   if ( botches > 0 ) {
					   u2->SendMessage(bi, _("|-- Total Botches: %d"), botches);
               }
				   if ( specialization > 0 ) 
				   {
					   u2->SendMessage(bi, _("|-- Specialization rerolls: %d"), specialization);
               }
				   u2->SendMessage(bi, _("+-- Total Successes: %d"), successes);
            }
				source.Reply("Rolled %d dice with a difficulty of %d:  %s", many, diff, message.c_str());
				if(total < 0){
					source.Reply("|-- * * * BOTCHED * * * ");
				}
				source.Reply("|-- Total: %d ", total);
				source.Reply("|-- Dice Rolled: %s ", results.c_str());
				if ( botches > 0 ) {
					source.Reply("|-- Total Botches: %d", botches);
				}
				if ( specialization > 0 ) 
				{
					source.Reply("|-- Specialization rerolls: %d", specialization);
				}
				source.Reply("+-- Total Successes: %d", successes);
			}
			else
			{
				u->SendMessage(bi, _("%s Rolled %d dice with a difficulty of %d: %d  %s"), u->nick.c_str(), many, diff, successes, message.c_str());
				if(total < 0)
				   u->SendMessage(bi, _("|-- * * * BOTCHED * * * "));
				u->SendMessage(bi, _("|-- Rolled %d dice with a difficulty of %d: %d  %s"), many, diff, successes, message.c_str());
				u->SendMessage(bi, _("|-- Rolled: %s "), results.c_str());
				if ( botches > 0 )
					u->SendMessage(bi, _("|-- Botches: %d"), botches);
				if ( specialization > 0 )
					u->SendMessage(bi, _("|-- Specialization reroll: %d"), specialization);
				u->SendMessage(bi, _("+-- Successes: %d"), botches);
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
		source.Reply(_("Roll World of darkness dice.\n"
			" \n"
			"This command is to simulate the rolling of dice for the\n"
			"World Of Darkness dice system. Dice are rolled against\n"
			"a target difficulty specified by the difficulty paramater.\n"
			"Up to 30 dice may be rolled against this difficulty.\n"
			" \n"
			"Both successes and botches are tallied and the number of\n"
			"rerolls allowed for specialization is computed and\n"
			"displayed.\n"
			" \n"
			"The [optional where] paramater is as implied an\n"
			"optional paramater. It specifies a location in addition\n"
			"to the user to which output is sent. This can be either\n"
			"a channel or another username. No error checking is\n"
			"performed on this parameter and if invalid information\n"
			"is supplied it will simply output to the user alone.\n"
			" \n"
			"Note, that as with all RPGServ commands, to use the \n"
			"descriptive text you must supply the where value for \n"
			"the secondary recipient of the results (be that a \n"
			"channel or a person).\n"
			" \n"));
		return true;
	}
};

class RSWod : public Module
{
	CommandRSWod commandrswod;

 public:
	RSWod(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrswod(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSWod)
