This is a simple desktop application for testing and optimizing sqlite3 workflows.

This codebase is made to be simple and portable. The way that I'm using it is to work out sqlite3 strategies on my workstation for a mobile app that I'm working on.

Once I'm happy with how the database works, I can simply cut and paste the following files into the other codebase:
  src/**
  extern/sqlite3/**

Anyone is welcome to use this codebase for anything. I don't expect anything in return.
Look for #defines - there's conditional compilation statements for everything that prints to the console.
Read everything in the notes directory.

The sorting method used to rank search results is a placeholder for application-specific methods.
The placeholder functions are:
  Search::calculateParagraphScores()
	Search::rankParagraphs()
	Search::rankParagraphIds()




Extended notes:

I had to use pthreads instead of threads to realize a measurable benefit from multithreading on my linux workstation.
I used pthreads' realtime FIFO thread priority with the highest level of schedule priority which... isn't necessary. Haha. 

Using two threads and running the statements in parallel is a little bit faster than combining all of the sql query statements into one (which you can see commmented out in src/DDL/table_words_to_paragraphs.h) single-threaded statement.

If I wanted to make this faster, I would put the words table and the words to paragraphs table into memory, or create a custom implimentation of those two tables in memory directly (not use sqlite at all for those two tables).
In fact, I think a custom implimentation is exactly the way to do it, but that's something to think more about later on.
Maybe I'll wait until iPhones have 2 more GPU cores to do that - thats ~4GHz. 4GHz without system interupts. Hmm. It's worth a try.

I wanted these queries to be ~1/10th of a frame on mobile. 
90fps = ~11,111 microseconds, so 1/10th of a frame would be 1,111 microseconds.

On my workstation, I'm meeting that timeframe for basically anything other than single-letter queries, which can take 2,000-3,000 microseconds.
Full word queries tend to be ~600 microseconds when built in Release mode -O3.

Assuming that an average mobile device is 1/2 the speed of my workstation:
(~600 * 2) / 11,000 = ~10.8% of a frame for a full word query.

On the iPhone 16 and Google Pixel 9, I think I met my goal! I have yet to test this on those platforms, but I'm feeling very optimistic.

On mobile I'll be querying results in 'chunks' anyways. Maybe 3-5 results will actually be able to fit on the page.
