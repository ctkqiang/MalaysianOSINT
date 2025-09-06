#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/memory.h"
#include "../include/social.h"

#define MHD_CONTENT_READER_END_WITHOUT_FLUSH 0
#define MHD_CONTENT_READER_END_OF_STREAM 0

/**
 * CURL写回调函数，用于接收HTTP响应数据并存储到用户定义的结构体中
 * 
 * @param ptr 指向接收到的数据的指针
 * @param size 每个数据元素的大小（字节数）
 * @param nmemb 数据元素的数量
 * @param userdata 指向用户数据的指针，这里应该是semak_mule_response结构体
 * 
 * @return 返回实际处理的数据大小（字节数），如果处理失败返回0
 */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct memory *resp = (struct memory *)userdata;

    // 重新分配内存以容纳新接收的数据
    char *new_data = realloc(resp -> data, resp -> size + total + 1);

    if (!new_data) {
        fprintf(stderr, "realloc failed\n");
        return 0;
    }

    resp -> data = new_data;

    // 将新数据复制到已分配的内存空间中
    memcpy(&(resp -> data[resp -> size]), ptr, total);
    
    resp -> size += total;
    resp -> data[resp -> size] = '\0';
    
    return total;
}

/**
 * 初始化CURL库
 * 
 * 该函数用于在程序启动时初始化CURL库，为后续的HTTP请求做准备。
 * 它会初始化所有的CURL功能，包括所有支持的协议和SSL功能。
 * 这个函数应该在程序开始时调用，并且在程序生命周期内只调用一次。
 * 
 * @note 这个函数不是线程安全的，应该在创建任何线程之前调用
 * @note 调用此函数后，必须在程序结束前调用cleanup_curl()进行清理
 */
void init_curl() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

/**
 * 清理CURL库
 * 
 * 该函数用于在程序结束时清理CURL库所使用的所有资源。
 * 它会释放init_curl()分配的所有资源，包括全局数据和内存。
 * 这个函数应该在程序结束时调用，并且在程序生命周期内只调用一次。
 * 
 * @note 这个函数不是线程安全的，应该在所有CURL操作完成后调用
 * @note 调用此函数后，如果还需要使用CURL，需要重新调用init_curl()
 */
void cleanup_curl() {
    curl_global_cleanup();
}

/**
 * HTTP响应数据分块发送回调函数
 * 
 * 该函数用于处理HTTP响应数据的分块发送。它会逐个检查社交媒体目标，
 * 并生成对应的用户名检查结果。每次调用会处理一个社交媒体平台的检查结果。
 * 
 * @param cls 指向用户自定义数据的指针，这里是social_state结构体
 * @param pos 当前数据块的位置（未使用）
 * @param buf 用于存储要发送的数据的缓冲区
 * @param max 缓冲区的最大大小
 * 
 * @return 返回写入缓冲区的字节数，如果没有更多数据返回MHD_CONTENT_READER_END_OF_STREAM
 * 
 * @note 这个函数会被HTTP服务器框架重复调用，直到所有目标都被检查完毕
 */
ssize_t callback_send_chunk(void *cls, uint64_t pos, char *buf, size_t max) {
    struct social_state *state = (struct social_state *)cls;

    if (state -> current_target >= targets_count) return MHD_CONTENT_READER_END_OF_STREAM;
    
    size_t n = 0;

    char url[512];
    snprintf(url, sizeof(url), targets[state -> current_target].url_template, state -> username);

    int found = check_username(&targets[state -> current_target], state -> username);

    if (found) {
        n = snprintf(buf, max, "[+] %s: username exists at %s\n", targets[state -> current_target].name, url);
    } else {
        n = snprintf(buf, max, "[-] %s: username not found\n", targets[state -> current_target].name);
    }

    state -> current_target++;
    
    return n;
}

/**
 * 检查用户名在特定社交媒体平台上是否存在
 * 
 * 该函数通过HTTP请求检查指定的用户名在给定的社交媒体平台上是否存在。
 * 它会构造完整的URL，发送HTTP请求，并分析响应来判断用户名是否存在。
 * 
 * @param target 指向SocialTarget结构体的指针，包含平台名称和URL模板
 * @param username 要检查的用户名
 * 
 * @return 如果用户名存在返回1，不存在返回0
 * 
 * @note 函数使用CURL库发送HTTP请求，超时时间设置为3秒
 * @note 函数会检查HTTP状态码和响应内容中是否包含用户名
 * @note 如果请求失败会打印错误信息到标准错误输出
 */
int check_username(const SocialTarget *target, const char *username) {
    CURL *curl;
    CURLcode res;
    
    struct memory chunk = {0};
    int found = 0;

    curl = curl_easy_init();

    if(curl) {
        char full_url[512];
        snprintf(full_url, sizeof(full_url), target->url_template, username);

        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "[%s] curl error: %s\n", target -> name, curl_easy_strerror(res));
        } else {
            long status;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

            if(status == 200 && strstr(chunk.data, username) != NULL) {
                printf("[+] %s: username exists at %s\n", target -> name, full_url);
                found = 1;
            } else {
                printf("[-] %s: username not found\n", target -> name);
            }
        }

        curl_easy_cleanup(curl);
        free(chunk.data);
    }

    return found;
}

SocialTarget targets[] = {
    // Global / previously added
    {"GitHub", "https://github.com/%s"},
    {"X", "https://x.com/%s"},
    {"Reddit", "https://www.reddit.com/user/%s"},
    {"Instagram", "https://www.instagram.com/%s"},
    {"TikTok", "https://www.tiktok.com/@%s"},
    {"Telegram", "https://t.me/%s"},
    {"Facebook", "https://www.facebook.com/%s"},
    {"YouTube", "https://www.youtube.com/%s"},
    {"LinkedIn", "https://www.linkedin.com/in/%s"},
    {"Snapchat", "https://www.snapchat.com/add/%s"},
    {"Pinterest", "https://www.pinterest.com/%s"},
    {"Medium", "https://medium.com/@%s"},
    {"Twitch", "https://www.twitch.tv/%s"},
    {"Discord", "https://discord.com/users/%s"},
    {"Flickr", "https://www.flickr.com/people/%s"},
    {"Spotify", "https://open.spotify.com/user/%s"},
    {"Steam", "https://steamcommunity.com/id/%s"},
    {"Vimeo", "https://vimeo.com/%s"},
    {"WordPress", "https://%s.wordpress.com"},
    {"GitLab", "https://gitlab.com/%s"},
    
    // Russia
    {"VK", "https://vk.com/%s"},
    {"Odnoklassniki", "https://ok.ru/profile/%s"},
    {"Yandex", "https://yandex.ru/%s"},
    {"Mail.ru", "https://my.mail.ru/%s"},
    
    // EU / Western
    {"Xing", "https://www.xing.com/profile/%s"},
    {"ResearchGate", "https://www.researchgate.net/profile/%s"},
    {"StackOverflow", "https://stackoverflow.com/users/%s"},
    {"Badoo", "https://badoo.com/%s"},
    
    // USA
    {"Quora", "https://www.quora.com/profile/%s"},
    {"Dribbble", "https://dribbble.com/%s"},
    {"Behance", "https://www.behance.net/%s"},
    {"Goodreads", "https://www.goodreads.com/%s"},
    {"DeviantArt", "https://www.deviantart.com/%s"},
    {"Patreon", "https://www.patreon.com/%s"},
    {"SoundCloud", "https://soundcloud.com/%s"},
    
    // China
    {"Weibo", "https://weibo.com/%s"},
    {"WeChat", "https://wechat.com/%s"},
    {"Douyin", "https://www.douyin.com/user/%s"},
    {"Bilibili", "https://space.bilibili.com/%s"},
    {"QQ", "https://user.qzone.qq.com/%s"},
    {"Zhihu", "https://www.zhihu.com/people/%s"},
    
    // Additional Global Platforms
    {"WhatsApp", "https://wa.me/%s"},
    {"Signal", "https://signal.me/#p/%s"},
    {"Viber", "https://viber.com/%s"},
    {"Skype", "https://join.skype.com/invite/%s"},
    {"Zoom", "https://zoom.us/profile/%s"},
    {"Microsoft Teams", "https://teams.microsoft.com/profile/%s"},
    {"Slack", "https://%s.slack.com"},
    {"Mastodon", "https://mastodon.social/@%s"},
    {"Threads", "https://www.threads.net/@%s"},
    {"BlueSky", "https://bsky.app/profile/%s"},
    {"Clubhouse", "https://www.clubhouse.com/@%s"},
    {"OnlyFans", "https://onlyfans.com/%s"},
    {"Substack", "https://%s.substack.com"},
    {"Ghost", "https://%s.ghost.io"},
    {"Tumblr", "https://%s.tumblr.com"},
    {"LiveJournal", "https://%s.livejournal.com"},
    {"Blogger", "https://%s.blogspot.com"},
    {"Wix", "https://%s.wixsite.com"},
    {"Squarespace", "https://%s.squarespace.com"},
    {"Linktree", "https://linktr.ee/%s"},
    {"About.me", "https://about.me/%s"},
    {"Carrd", "https://%s.carrd.co"},
    
    // Professional/Business
    {"AngelList", "https://angel.co/%s"},
    {"Crunchbase", "https://www.crunchbase.com/person/%s"},
    {"ProductHunt", "https://www.producthunt.com/@%s"},
    {"Kaggle", "https://www.kaggle.com/%s"},
    {"HackerRank", "https://www.hackerrank.com/%s"},
    {"LeetCode", "https://leetcode.com/%s"},
    {"CodePen", "https://codepen.io/%s"},
    {"Replit", "https://replit.com/@%s"},
    {"Glitch", "https://glitch.com/@%s"},
    {"Observable", "https://observablehq.com/@%s"},
    {"Notion", "https://www.notion.so/%s"},
    {"Figma", "https://www.figma.com/@%s"},
    {"Adobe Portfolio", "https://%s.myportfolio.com"},
    {"Artstation", "https://www.artstation.com/%s"},
    {"500px", "https://500px.com/p/%s"},
    {"Unsplash", "https://unsplash.com/@%s"},
    {"Shutterstock", "https://www.shutterstock.com/g/%s"},
    
    // Gaming
    {"Xbox Live", "https://account.xbox.com/en-us/profile?gamertag=%s"},
    {"PlayStation", "https://psnprofiles.com/%s"},
    {"Nintendo", "https://www.nintendo.com/us/switch/friends/%s"},
    {"Epic Games", "https://epicgames.com/site/en-US/profile/%s"},
    {"Battle.net", "https://playoverwatch.com/en-us/career/pc/%s"},
    {"Origin", "https://www.origin.com/profile/%s"},
    {"Uplay", "https://club.ubisoft.com/en-US/profile/%s"},
    {"Roblox", "https://www.roblox.com/users/%s"},
    {"Minecraft", "https://namemc.com/profile/%s"},
    {"Fortnite Tracker", "https://fortnitetracker.com/profile/all/%s"},
    {"PUBG Tracker", "https://pubgtracker.com/profile/pc/%s"},
    {"League of Legends", "https://op.gg/summoner/userName=%s"},
    {"CS:GO Stats", "https://csgostats.gg/player/%s"},
    
    // Music/Audio
    {"Apple Music", "https://music.apple.com/profile/%s"},
    {"Deezer", "https://www.deezer.com/en/profile/%s"},
    {"Last.fm", "https://www.last.fm/user/%s"},
    {"Bandcamp", "https://%s.bandcamp.com"},
    {"Mixcloud", "https://www.mixcloud.com/%s"},
    {"Audiomack", "https://audiomack.com/%s"},
    {"ReverbNation", "https://www.reverbnation.com/%s"},
    {"DistroKid", "https://distrokid.com/hyperfollow/%s"},
    {"Genius", "https://genius.com/%s"},
    {"Discogs", "https://www.discogs.com/user/%s"},
    
    // Dating/Social
    {"Tinder", "https://tinder.com/@%s"},
    {"Bumble", "https://bumble.com/%s"},
    {"Hinge", "https://hinge.co/%s"},
    {"Match", "https://www.match.com/%s"},
    {"eHarmony", "https://www.eharmony.com/%s"},
    {"OkCupid", "https://www.okcupid.com/profile/%s"},
    {"POF", "https://www.pof.com/viewprofile.aspx?profile_id=%s"},
    {"Zoosk", "https://www.zoosk.com/profile/%s"},
    {"Coffee Meets Bagel", "https://coffeemeetsbagel.com/%s"},
    {"Grindr", "https://www.grindr.com/%s"},
    
    // Forums/Communities
    {"4chan", "https://boards.4chan.org/%s"},
    {"8kun", "https://8kun.top/%s"},
    {"Something Awful", "https://forums.somethingawful.com/member.php?action=getinfo&username=%s"},
    {"NeoGAF", "https://www.neogaf.com/members/%s"},
    {"ResetEra", "https://www.resetera.com/members/%s"},
    {"Hacker News", "https://news.ycombinator.com/user?id=%s"},
    {"Lobsters", "https://lobste.rs/u/%s"},
    {"Slashdot", "https://slashdot.org/~%s"},
    {"Digg", "https://digg.com/@%s"},
    {"StumbleUpon", "https://www.stumbleupon.com/stumbler/%s"},
    
    // Regional/Country Specific
    // Japan
    {"Niconico", "https://www.nicovideo.jp/user/%s"},
    {"Pixiv", "https://www.pixiv.net/users/%s"},
    {"LINE", "https://line.me/R/ti/p/%s"},
    {"Mixi", "https://mixi.jp/show_friend.pl?id=%s"},
    {"2channel", "https://2ch.net/%s"},
    
    // Korea
    {"KakaoTalk", "https://open.kakao.com/o/%s"},
    {"Naver", "https://blog.naver.com/%s"},
    {"Cyworld", "https://www.cyworld.com/home/%s"},
    {"DC Inside", "https://gall.dcinside.com/%s"},
    
    // India
    {"ShareChat", "https://sharechat.com/profile/%s"},
    {"Moj", "https://www.moj.com/@%s"},
    {"Josh", "https://www.josh.com/@%s"},
    {"Roposo", "https://www.roposo.com/%s"},
    
    // Brazil
    {"Orkut", "https://orkut.br.com/%s"},
    {"Kwai", "https://www.kwai.com/@%s"},
    
    // Middle East
    {"Imo", "https://imo.im/%s"},
    {"ToTok", "https://totok.ai/%s"},
    
    // Europe
    {"VKontakte Music", "https://vk.com/audio%s"},
    {"Hi5", "https://hi5.com/friend/displayProfile.do?userid=%s"},
    {"Tagged", "https://www.tagged.com/profile/%s"},
    {"MeetMe", "https://www.meetme.com/%s"},
    {"IMVU", "https://www.imvu.com/catalog/web_search.php?keywords=%s"},
    
    // Video Platforms
    {"Dailymotion", "https://www.dailymotion.com/%s"},
    {"Metacafe", "https://www.metacafe.com/channels/%s"},
    {"Veoh", "https://www.veoh.com/users/%s"},
    {"Break", "https://www.break.com/user/%s"},
    {"Vine", "https://vine.co/%s"},
    {"IGTV", "https://www.instagram.com/tv/%s"},
    {"YouTube Shorts", "https://youtube.com/shorts/%s"},
    {"Loom", "https://www.loom.com/%s"},
    {"Wistia", "https://%s.wistia.com"},
    
    // Livestreaming
    {"Periscope", "https://www.pscp.tv/%s"},
    {"YouNow", "https://www.younow.com/%s"},
    {"Live.me", "https://www.liveme.com/v/%s"},
    {"Bigo Live", "https://www.bigolive.tv/%s"},
    {"StreamLabs", "https://streamlabs.com/%s"},
    {"OBS", "https://obsproject.com/%s"},
    {"Mixer", "https://mixer.com/%s"},
    {"DLive", "https://dlive.tv/%s"},
    {"Trovo", "https://trovo.live/%s"},
    {"Facebook Gaming", "https://www.facebook.com/gaming/%s"},
    {"YouTube Gaming", "https://gaming.youtube.com/channel/%s"},
    
    // Messaging Apps
    {"Kik", "https://kik.me/%s"},
    {"Wickr", "https://wickr.com/%s"},
    {"Wire", "https://wire.com/@%s"},
    {"Element", "https://matrix.to/#/@%s"},
    {"Session", "https://getsession.org/%s"},
    {"Briar", "https://briarproject.org/%s"},
    {"Jami", "https://jami.net/%s"},
    {"Tox", "https://tox.chat/%s"},
    {"Ricochet", "https://ricochet.im/%s"},
    
    // Blockchain/Crypto
    {"Steemit", "https://steemit.com/@%s"},
    {"Hive", "https://hive.blog/@%s"},
    {"Mirror", "https://mirror.xyz/%s"},
    {"Lens Protocol", "https://lenster.xyz/u/%s"},
    {"Farcaster", "https://warpcast.com/%s"},
    {"BitClout", "https://bitclout.com/u/%s"},
    {"Rally", "https://rally.io/%s"},
    {"Foundation", "https://foundation.app/@%s"},
    {"SuperRare", "https://superrare.co/%s"},
    {"OpenSea", "https://opensea.io/%s"},
    {"Rarible", "https://rarible.com/%s"},
    
    // Academic/Research
    {"ORCID", "https://orcid.org/%s"},
    {"Google Scholar", "https://scholar.google.com/citations?user=%s"},
    {"Academia.edu", "https://independent.academia.edu/%s"},
    {"Mendeley", "https://www.mendeley.com/profiles/%s"},
    {"Zotero", "https://www.zotero.org/%s"},
    {"Papers", "https://papers.ssrn.com/sol3/cf_dev/AbsByAuth.cfm?per_id=%s"},
    {"PubMed", "https://pubmed.ncbi.nlm.nih.gov/?term=%s"},
    {"arXiv", "https://arxiv.org/search/?searchtype=author&query=%s"},
    {"bioRxiv", "https://www.biorxiv.org/search/%s"},
    {"Coursera", "https://www.coursera.org/instructor/%s"},
    {"edX", "https://www.edx.org/bio/%s"},
    {"Udemy", "https://www.udemy.com/user/%s"},
    {"Khan Academy", "https://www.khanacademy.org/profile/%s"},
    
    // Fitness/Health
    {"Strava", "https://www.strava.com/athletes/%s"},
    {"MyFitnessPal", "https://www.myfitnesspal.com/profile/%s"},
    {"Fitbit", "https://www.fitbit.com/user/%s"},
    {"Garmin Connect", "https://connect.garmin.com/modern/profile/%s"},
    {"Nike Run Club", "https://www.nike.com/nrc/profile/%s"},
    {"Adidas Running", "https://www.runtastic.com/users/%s"},
    {"Peloton", "https://members.onepeloton.com/members/%s"},
    
    // Travel
    {"TripAdvisor", "https://www.tripadvisor.com/members/%s"},
    {"Booking.com", "https://www.booking.com/profiles/%s"},
    {"Airbnb", "https://www.airbnb.com/users/show/%s"},
    {"Couchsurfing", "https://www.couchsurfing.com/people/%s"},
    {"Nomad List", "https://nomadlist.com/@%s"},
    {"Foursquare", "https://foursquare.com/%s"},
    {"Swarm", "https://swarmapp.com/user/%s"},
    {"GetYourGuide", "https://www.getyourguide.com/profiles/%s"},
    
    // Food/Lifestyle
    {"Yelp", "https://www.yelp.com/user_details?userid=%s"},
    {"Zomato", "https://www.zomato.com/users/%s"},
    {"OpenTable", "https://www.opentable.com/profiles/%s"},
    {"Allrecipes", "https://www.allrecipes.com/cook/%s"},
    {"Food52", "https://food52.com/users/%s"},
    {"Epicurious", "https://www.epicurious.com/profiles/%s"},
    {"Tasty", "https://tasty.co/@%s"},
    
    // E-commerce/Marketplace
    {"Etsy", "https://www.etsy.com/people/%s"},
    {"eBay", "https://www.ebay.com/usr/%s"},
    {"Amazon", "https://www.amazon.com/gp/profile/amzn1.account.%s"},
    {"Depop", "https://www.depop.com/%s"},
    {"Poshmark", "https://poshmark.com/closet/%s"},
    {"Mercari", "https://www.mercari.com/u/%s"},
    {"Vinted", "https://www.vinted.com/member/%s"},
    {"ThredUP", "https://www.thredup.com/%s"},
    {"Vestiaire Collective", "https://www.vestiairecollective.com/members/%s"},
    {"Grailed", "https://www.grailed.com/users/%s"},
    {"StockX", "https://stockx.com/users/%s"},
    {"GOAT", "https://www.goat.com/profile/%s"},
    
    // News/Media
    {"Flipboard", "https://flipboard.com/@%s"},
    {"Pocket", "https://getpocket.com/@%s"},
    {"Feedly", "https://feedly.com/%s"},
    {"NewsBlur", "https://newsblur.com/social/#/%s"},
    {"AllSides", "https://www.allsides.com/users/%s"},
    
    // Miscellaneous
    {"Archive.org", "https://archive.org/details/@%s"},
    {"Internet Archive", "https://archive.org/details/@%s"},
    {"Wayback Machine", "https://web.archive.org/web/*/%s"},
    {"Have I Been Pwned", "https://haveibeenpwned.com/account/%s"},
    {"Gravatar", "https://gravatar.com/%s"},
    {"Keybase", "https://keybase.io/%s"},
    {"OpenPGP", "https://keys.openpgp.org/search?q=%s"},
    {"ProtonMail", "https://protonmail.com/%s"},
    {"Tutanota", "https://tutanota.com/%s"},
    {"Temp Mail", "https://temp-mail.org/en/%s"},
    {"Guerrilla Mail", "https://www.guerrillamail.com/%s"}
};

size_t targets_count = sizeof(targets) / sizeof(targets[0]);