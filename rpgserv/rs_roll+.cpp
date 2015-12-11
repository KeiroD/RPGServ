/*
 * RPGServ roll functions
 *
 */

#include "module.h"

class CommandRSRollPlus : public Command
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
	CommandRSRollPlus(Module *creator) : Command(creator, "rpgserv/roll+", 2, 4)
	{
		this->SetDesc(_("Rolls a specifed set of dice."));
		this->SetSyntax(_("\002\037<dice>\037 \037+<flags>\037 \037[where]\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &dice = params[0];
		const Anope::string &flags = params[1];
		const Anope::string &chan = params.size() > 2 ? params[2] : "";
		const Anope::string &message = params.size() > 3 ? params[3] : "";
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		int sidemax = Config->GetModule("rpgserv")->Get<int>("sidemax","100");
		Anope::string target;
		Anope::string dicemessage;
		Anope::string result;
		int X;
		int Y;
		int myroll;
		int myroll2;
		int i;
		int total = 0;
		User *u2 = NULL;
		srand(time(NULL));
		int dicerolls[10000];
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
		int sorted = flags.find_ci("s") != Anope::string::npos ? 1 : 0;
		int reroll = flags.find_ci("r") != Anope::string::npos ? 1 : 0;

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

		//need to parse the dice, but first lets catch for fun items
      if ( dice.find_first_of_ci("[abcefghijklmnopqrstuvwxyz]{}|\\<>/,.?:;!-_=+@#$%^&*()`~") != Anope::string::npos )
      {
         source.Reply(_("Dice must be in the format of XdY where X and Y are positive integers only."));
         return;
      }
		if ( dice.equals_ci("%")  || dice.find("%") != Anope::string::npos)
		{
			source.Reply(_("Please use 100 instead of '%%'."));
			return;
		}
		if ( dice.equals_ci("d") )
		{
			source.Reply(_("You must use the XdY format for dice."));
			return;
		}
		if ( dice.c_str()[0] == 'd' )
		{
			X = 1;
			Anope::string temp = dice.substr(1, dice.length());
			if(temp.is_pos_number_only())
 				Y = convertTo<int>(dice.substr(1, dice.length()));
			else
			{
				source.Reply(_("You must use the XdY format for dice."));
				return;
			}
		}
		else if ( int pos = dice.find_first_of('d') )
		{
			if ( pos == 0) 
				X = 1;
			else 
				X = convertTo<int>(dice.substr(0,pos));
			Anope::string Ys = dice.substr(pos + 1, dice.length());
			if ( !Ys.is_pos_number_only()  || Ys.empty() )
			{
				source.Reply(_("You must use the XdY format for dice."));
				return;
			}
			Y = convertTo<int>(dice.substr(pos + 1, dice.length()));
			if( X > dicemax || X < 1 ) {
				source.Reply(_("Number of dice must be between 1 and %d."),dicemax);
				this->SendSyntax(source);
				return;
			}
			if( Y > sidemax || Y < 1 ) {
				source.Reply(_("Number of sides must be between 0 and %s."),sidemax);
				this->SendSyntax(source);
				return;
			}
      }
		for(i=0; i<X; i++)
		{	
			myroll2 = 0;
			myroll = roll(1,Y);
			if(reroll == 1)
			{
				while(myroll==Y)
				{
					myroll2 = myroll2 + myroll;
					myroll = roll(1,Y);
				}
			}
			myroll2 = myroll2 + myroll;
			dicerolls[i]=myroll2;
			total = total + myroll2;
		}
		if(sorted == 1)
		{
			std::sort(dicerolls, dicerolls + X);
		}
		for(i=0; i<X; i++)
		{
			result.append(" ");
			result.append(stringify(dicerolls[i]));
		}

		Log(LOG_COMMAND,source, this) << "with the paramaters of:" << dice << " " << flags << " " << chan << " " << message;
		if (!target.empty())
		{
			if (!u2)
			{
				IRCD->SendPrivmsg(bi, target.c_str(), "%s Rolled %s for %d: %s", u->nick.c_str(), dice.c_str(), total, message.c_str());
				IRCD->SendPrivmsg(bi, target.c_str(), "+-- Individual Rolls %s", result.c_str());
			}
			else
			{
				u2->SendMessage(bi, _("%s Rolled %s for %d: %s"),u->nick.c_str(), dice.c_str(), total, message.c_str());
				u2->SendMessage(bi, _("+-- Individual Rolls %s"), result.c_str());
			}
			source.Reply("Rolled %s for %d: %s",dice.c_str(), total, message.c_str());
			source.Reply("+-- Individual Rolls %s", result.c_str());

		}
		else
		{
			u->SendMessage(bi, _("%s Rolled %s for %d: %s"), u->nick.c_str(), dice.c_str(), total, message.c_str());
			u->SendMessage(bi, _("+-- Individual Rolls %s"), result.c_str());
		}
		return;
	}
	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","30");
		int sidemax = Config->GetModule("rpgserv")->Get<int>("sidemax","100");
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Roll dice.  \n"
			"This command is to simulate the rolling of polyhedral\n"
			"dice. it will handle dice with up to %d sides, &\n"
			"quantities of dice up to %d. (Values in\n"
			"excess of these are automatically degraded to the\n"
			"maximum value.)\n"
			" \n"
			"The <dice> argument is required and specfies what\n"
			"dice to roll and any additional functions which need\n"
			"to be preformed with said dice. Dice format is in\n"
			"common dice (XdY) notation.\n"
			" \n"
			"The +<flags> argument is required.  If you do not wish to\n"
			"Specify any flags use a dash '-' for no flags.  You can\n"
			"specify multiple flags by including them all in the same string.\n"
			"It must start with the '+'.\n"
			"Flags are:\n"
			"	- for none\n"
		        "	R for reroll max values\n"
		        "	S for sort rolls (display only).\n"
			" \n"
			"The results include a list of what individual rolls were done.\n"
			" \n"
			"The [where] parameter is as implied, an optional paramater.\n"
	       		"It specifies a location in addition to the user to which output\n"
			"is sent. This can be either a channel or another username. \n"
			"Minimal error checking is performed on this parameter and if \n"
			"invalid information is supplied it will simply output to the \n"
			"user alone.\n"
			" \n"
			"The [message] parameter is an optional messed to be showen when \n"
			"the roll results are displayed.\n"
			" \n"),sidemax,dicemax);
		return true;
	}
};

class RSRollPlus : public Module
{
	CommandRSRollPlus commandrsrollplus;

 public:
	RSRollPlus(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrsrollplus(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSRollPlus)
