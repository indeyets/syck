require 'okay/news'

# Wrapping strings for display
class String
  def wordwrap( len )
    gsub( /\n\s*/, "\n\n" ).gsub( /.{#{len},}?\s+/, "\\0\n" )
  end
end

news = YAML::parse( DATA )
if Okay.validate_node( news )
    puts "** !okay/news validated **"
    news = news.transform
    news.items.each { |item|
      puts "-- #{ news.title } @ #{ item.pubTime } --"
      puts item.description.wordwrap( 70 ).gsub!( /^/, '   ' )
      puts 
    }
end


__END__
--- %YAML:1.0 !okay/news
title: whytheluckystiff.net
link: http://www.whytheluckystiff.net/
description: Home remedies for braindeath.
updatePeriod: 00:60.00
items:
  - !okay/news/item
    pubTime: 2002-10-23T09:03:40.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/23#1035385420 
    description: >
      Considering the
      "discussion"="http://philringnalda.com/archives/002359.php"
      around the Web about saving RSS bandwidth, I checked the
      size of my RSS feed: 33k.  And my YAML feed: 15k.  A big
      part of that is my content included twice for the
      "content:encoded"="http://web.resource.org/rss/1.0/modules/content/"
      tag.  Which brings up another valid point in favor of YAML
      feeds.  **YAML doesn't have entity-encoding issues.**  XML
      users: can you imagine?

  - !okay/news/item
    pubTime: 2002-10-22T23:46:57.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/22#1035352017 
    description: >
      Last night I hung out at this hotel with my relatives, all
      in town to see
      "Dot"="http://www.whytheluckystiff.net/arch/2002/10/20#1035131109".
       I haven't wanted to post here on my site all the events
      concerning her death, even though I
      "abandoned"="http://www.whytheluckystiff.net/arch/2002/10/07#1034018848"
      my paper journal to write on this site.  I think I need to
      flag some entries as private.  Because it's too wierd
      having a site where one minute I'm jabbering about YAML
      ideas and the next I'm going into the details of ovarian
      cancer or the rifts in my family life.  Sure, I want both
      documented.  But I think I can tell what entries are meant
      for you and what is meant for me.
      
      I'm going to write anyway, though.  Under her closed eyes,
      she went away on Sunday night at 8:12 PM.  She did go
      naturally, as we all wished.  You know, I've never had a
      life like I have right now.  Feeling her with me now. 
      Believing that she is somewhere now.  Why do I insist on
      thinking that?  They all think she's been ushered off a
      plane and onto some tropical beach, a part of some Carnival
      cruise that you get for free after you die.  Run by the
      Carnival people who died and their heaven is continuing
      their employment.  (Why am I telling heaven jokes?)
      
      I feel love, so I think about her.  Her death was a sunset.
       As the sun is buried, it still casts the colors upon the
      world for a while longer.  I see those colors in my life
      now.  It's either her literal dispersement into the air or
      she is simply so joyful now that I can feel her happiness
      from my vantage point.  Can I just tell you: it's so
      incredible to know someone who had no agenda, who never got
      offended, who wielded such power but never used it against
      anyone?
      
      So, the hotel.  We went downstairs and lounged in the
      jacuzzi.  Talked about work, baseball, Dot and Ray.  We did
      some dives into the pool.
      
      One guy walked in with his pecs and abs with his chick and
      messed around in the pool.  Dancing around with each other
      and playing naughty, splashy pool games with each other. 
      Suddenly they were in the jacuzzi with all of us.  It was
      whack.  It was _The Bachelor_.  I think I even said, "Hey,
      are you *The Bachelor*?"  He went and told on us for having
      too many people in the pool and *especially* in the
      jacuzzi.  My aunt and uncle had to move hotels.
      
      And that's when I started to question the beauty of life
      again.

  - !okay/news/item
    pubTime: 2002-10-21T15:34:31.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/21#1035236071 
    description: >
      Today I've decided to rewrite the parser backend for
      yaml.rb.  The new parser will be called **Pill** and will
      also become the secret behind
      "reStructuredText"="http://docutils.sf.net/rst.html" for
      Ruby.  The reST team has put some excellent effort into
      documenting and producing code for their project.  YAML and
      reST share so much in common that I thought I could save a
      lot of time by abstracting the parser so that it can handle
      both.
      
      To give you an idea of what I mean, just look at the
      following Ruby code.  To create a parser, you merely extend
      the Pill class and define the meaning of your tokens:
      
      = class YamlParser < Pill
      =   # Define YAML's literal block
      =   def_token :block, :literal,
      =        [ '|' ], [ :entry ]
      =   # Define mapping indicator
      =   def_token :indicator, :mapping,
      =        [ ':', :space ]
      = end
      
      This is prototype code.  Hopefully it gives the picture
      though.  I define the specific syntax symbols and the
      parser sends them to event handlers which can construct
      native datatypes (YAML) or markup (reST).

  - !okay/news/item
    pubTime: 2002-10-21T13:25:25.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/21#1035228325 
    description: >
      Okay, so I just found an
      "FAQ"="http://otakuworld.com/games/saturn/vs_moves.htm" for
      **Vampire Saviour: World of Darkness**, the game I "raved
      about"="http://advogato.org/article/562.html" on Advogato. 
      The article was likely misplaced.  Thinking about it now I
      can't imagine why I posted it there, but I thought it would
      be fun.  Probably was fun.  Keep in mind that this is an
      old game now and since I don't say that in the article it
      probably sounds like I'm plugging some new, hot commercial
      game.  No way.  This game is dead.  And time for a
      resurrection!
      
      It turns out that the reason I can't find any information
      about **Vampire Saviour: World of Darkness** is because the
      game isn't really even called that.  Yes, that's the name
      on the arcade machine.  I wasn't paying attention to the
      title screen, though, which probably said **The Lord of
      Vampire**.  At any rate, I didn't see a single vampire in
      this whole game.  Not to mention that it was a bright and
      colorful game (not a World of Darkness as one might
      suppose).
      
      With all the love that I have in my bosom for this fine
      Capcom production, I feel that I must rename it for use in
      my private spheres of influence.  In my associations with
      humanity, I will now refer to this game as **The Organism
      of Life-giving Eternity**.  So it is.  So it is.

  - !okay/news/item
    pubTime: 2002-10-20T10:25:09.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/20#1035131109 
    description: >
      My grandmother has slipped into a coma and will likely close
      her life in the next few days.  We've actually had her much
      longer than we were supposed to.  It's been a wonderful
      time to be with her.
      
      Her name is Dot and I think of her often.  She's one of
      those people in my family tree that I really hope a lot of
      genes seep down from.  She spent her whole life fully
      devoted to her husband.  Their life was golf, Hawaii,
      gambling, their children.  Her husband now has Alzheimer's
      and she has taught us that Alzheimer's isn't a frightening
      or sad disease.  She helps us see how cute he is and how he
      still does remember who we are.  Maybe he doesn't recognize
      our faces, but he recognizes something.  And so it's up to
      us to recognize him back.
      
      Dot makes me laugh everytime I am with her.  One of my
      favorites was when she looked up at my mom and said, "I was
      in love with two black men."  She can say whatever she
      wants these days and she does.  She thinks her doctors are
      sexy.  She watches lots of baseball, a life-long Dodger's
      fan, but also general fan of the game.
      
      I should stop writing now.  I just can't think about it all
      today.  Approaching death has been so hard for her.  I just
      wish her death could be as natural as the rest of her life
      has been.

  - !okay/news/item
    pubTime: 2002-10-19T11:02:53.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/19#1035046973 
    description: >
      Watching the "RSS 1.0/2.0
      logomachy"="http://diveintomark.org/archives/2002/10/18.html#take_up_knitting"
      is evocative in light of a new "YAML
      equivalent"="http://whytheluckystiff.net/why.yml".  The
      struggle in the YAML world will be trying to steer people
      away from interleaving content.  (The tags with namespaces
      you see mixed in with the RSS tags.)  The struggle is: what
      is our answer?  See the collection of links at the end of
      "Mark's"="http://diveintomark.org/" posting, along with the
      "XSS
      Draft"="http://www.mplode.com/tima/archives/000126.html".

  - !okay/news/item
    pubTime: 2002-10-19T10:46:36.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/19#1035045996 
    description: >
      About distribution in the Ruby kingdom.  I think
      "CPAN"="http://search.cpan.org/" has put a bit of undue
      pressure on us.  I personally use CPAN for documentation
      and for comparing modules before downloading them.  I don't
      use CPAN to install modules.  I use the ports collection or
      (on Linux) the packaging system.
      
      Ruby has "RAA"="http://www.ruby-lang.org/en/raa.html".  RAA
      is less than what we need.  RAA has some great stuff.  You
      can access it through web services.  I just wish it had a
      more comprehensive search and extensive documentation for
      each module.  And package mirrors.  Make sure we don't
      loose our libraries.
      
      Two RAA replacements are in progress, both of which I took
      a close look at today.
      
      # "rpkg"="http://www.allruby.com/rpkg/" is great.  It's a
      Debian-like packaging system for Ruby modules.  I don't
      know exactly how they solve the mirroring issues and
      there's no accessibility to module docs and definitions
      online, but it's got some neat ideas.
      # "rubynet"="http://www.rubynet.org/" is a precocious
      project to do about anything you could want to do with Ruby
      packaging.  I'm still trying to decide if it's overkill. 
      Again, plenty of ideas to ease installation but no hints of
      making each package's docs available.
      
      I don't think Ruby users have a large problem installing
      modules.  Most modules are quite straightforward.  The
      bigger issue is organization and documentation.  Are we the
      only group who hasn't standardized on a doc format!?

  - !okay/news/item
    pubTime: 2002-10-19T01:25:46.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/19#1035012346 
    description: >
      I am aghast!  YAML.rb 0.44 is now in the FreeBSD ports
      collection!  Sitting right there in
      /usr/ports/devel/ruby-yaml!  It installs perfectly.
      
      There are two amazing things about this:
      
      # Stanislav Grozev did this without any provocation from
      me.
      # I've actually seen one of my software projects through to
      see some sort of general distribution and acceptance!
      
      Seriously, what a kick!  The ports collection is like the
      Hall of Fame for me.  I cvsup fresh ports at least weekly
      and I have for the last several years.  I probably
      shouldn't be making such a big deal out of it, but it's so
      rewarding to see that someone appreciates this library
      enough to help step it along for distribution.  *Thank you,
      Stanislav!*

  - !okay/news/item
    pubTime: 2002-10-18T13:50:57.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/18#1034970657 
    description: >
      I know there's some incredible uses for YAML and
      documentation, but it's difficult to grasp exactly what
      that is.  Stefan Schmiedl posted an idea to the Yaml-core
      list last month:
      
      > What I had in mind was a "layered" wiki approach, where
      you would organize YAML-documents to create a page.  You
      would edit the "nodes" of the page via usual wiki approach
      or modify the sequence collecting the docs.  At the end of
      the day you'd have a large collection of YAML-docs and a
      second collection of "organizers" collating them into
      pages.
      
      The time is soon coming where this will be available here
      on this site.  I'm thinking of something similiar to Wiki. 
      The difference would be that the content would be updated
      to YAML documents on the server in Yod format.  The
      documents could then be exported to HTML, man pages, CHM,
      PDF.  It would be Wiki, but with a real end toward polished
      documentation.
      
      I want to store organized text on this site in such a way
      that it can be removed from the site, distributable, and
      yet very easy to edit.

  - !okay/news/item
    pubTime: 2002-10-18T10:01:49.00-06:00 
    link: http://whytheluckystiff.net/arch/2002/10/18#1034956909 
    description: >
      **Coldplay: A Rush of Blood to the Head**
      
      Despite what the music press might say, I feel obliged to
      like whatever music I please to like.  And yet, when I read
      something about an album, I let it easily influence my
      listening.  For a few weeks at least.  If some poor review
      keeps me from hearing an album or I can't make it through
      the album a few times, then likely I've missed the chance
      to hear the music as the artist intended it.
      
      I don't know why I gave **A Rush of Blood to the Head** a
      chance really.  I remember being unimpressed with
      Coldplay's first effort.  Not to mention that the public's
      admiration of the band was rather discouraging.  But these
      days I find myself opening back up to **Weezer**, **Ben
      Kweller**, **Supergrass**.  Perhaps I have a diluted
      catalog, but I really enjoy the songs.  What can I say?
      
      The song "Clocks" struck me in a peculiar way.  The song
      simply turned my opinion.  I actually heard his voice as he
      intended.  I fancied his use of repetition.  I went back
      and listened again.  This song had a very warm emotion
      attached.  The disc continued to play and that's when I
      realized that all of Coldplay's songs have that same
      emotion spun throughout.
      
      Perhaps that's a strike against the band.  Their feeling
      fluctuates between songs, but they don't ever get
      terrifically angry.  Nor do they pity themselves much.  If
      they don't explore all of those avenues of thought, how can
      they very well be songwriters at all?  Coldplay simply
      shines like a dazzle of light across a country lake. 
      Constantly and slightly beautiful.
      
      I'm not the sort of person who can bore of natural beauty. 
      I don't imagine many people can look at a sunset and
      discard it thanks to the monotony of sunsets each day.  It
      turns out that Coldplay makes beautiful music, which fills
      a definite void in my collection.
