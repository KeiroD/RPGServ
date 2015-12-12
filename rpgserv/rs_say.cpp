/*
 * RPGServ say function
 */

#include "module.h"

class CommandRSSay : public Command
{
 public:
	CommandRSSay(Module *creator) : Command(creator, "rpgserv/say", 1, 2)
	{
		this->SetDesc(_("Says something"));
		this->SetSyntax(_("\002\037#channel\037 \037<message>\037\002"));
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &chan = params[0];
		const Anope::string &message = params.size() > 1 ? params[1] : "";
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		Anope::string target ="";
		User *u = source.GetUser();
		
		if (chan.c_str()[0] == '#')
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
			User *u2 = User::Find(chan, true);
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

		if (!message.empty())
		{
			//IRCD->SendPrivmsg(Source, Dest, data, ...)
			Log(LOG_COMMAND,source, this) << "to SAY  " << message.c_str() << " in " << target.c_str();
			IRCD->SendPrivmsg(bi, target.c_str(), message.c_str());
		}
		else {
			this->SendSyntax(source);
			return;
		}
		return;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Has RPGServ say something in a channel or to a specified nick.\n"
			"Note: This command is ALWAYS logged.\n"
			" \n"));
		return true;
	}
};

class RSSay : public Module
{
	CommandRSSay commandrssay;

 public:
	RSSay(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA), 
		commandrssay(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");

	}
};

MODULE_INIT(RSSay)
