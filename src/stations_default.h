#ifndef _stations_h_
#define _stations_h_


struct StationInfo {
  const char* name;
  const char* url;
};

// DLF https://www.deutschlandradio.de/streamingdienste-100.html
// https://www.internet-radio.com/
// FluxFM https://archiv.fluxfm.de/stream-urls-channels/

StationInfo defaultStations[] = { 
  {"Radio Bremen 1", "http://icecast.radiobremen.de/rb/bremeneins/live/mp3/128/stream.mp3"},
  {"Radio Bremen 2","http://icecast.radiobremen.de/rb/bremenzwei/live/mp3/128/stream.mp3"},
  {"Radio Bremen 4", "http://icecast.radiobremen.de/rb/bremenvier/live/mp3/128/stream.mp3"},
  {"DR P4 (Bornholm)", "http://live-icy.gss.dr.dk/A/A06L.mp3"},
  {"NDR2 (NDS)", "http://www.ndr.de/resources/metadaten/audio/m3u/ndr2.m3u"},
  {"NDR Info (HH)","http://www.ndr.de/resources/metadaten/audio/m3u/ndrinfo_hh.m3u"},
  {"WDR2 (OWL)", "http://wdr-wdr2-ostwestfalenlippe.icecastssl.wdr.de/wdr/wdr2/ostwestfalenlippe/mp3/128/stream.mp3"},
  {"WDR5", "https://wdr-wdr5-live.icecastssl.wdr.de/wdr/wdr5/live/mp3/128/stream.mp3"},
  {"Einslive", "http://www.wdr.de/wdrlive/media/einslive.m3u"},
  {"DLF", "http://st01.sslstream.dlf.de/dlf/01/128/mp3/stream.mp3"},
  {"DLF Kultur", "http://st02.sslstream.dlf.de/dlf/02/128/mp3/stream.mp3"},
  {"DLF Nova", "http://st03.sslstream.dlf.de/dlf/03/128/mp3/stream.mp3"},
  {"DLF DokDeb", "https://st04.sslstream.dlf.de/dlf/04/128/mp3/stream.mp3"},
  {"ffn", "http://player.ffn.de/radioffn.mp3"},
  {"8080s", "http://regiocast.streamabc.net/regc-80s80smweb2517500-mp3-192-1672667"},
  {"KCWR", "http://kcrw.streamguys1.com/kcrw_192k_mp3_on_air"},
  {"FluxFM Hamburg", "http://streams.fluxfm.de/flux-hamburg/mp3-320/audio/"},
  {"FluxFM Alternative", "http://streams.fluxfm.de/alternative/mp3-320/streams.fluxfm.de/"},
  {"FluxFM 80er", "http://streams.fluxfm.de/80er/mp3-320/streams.fluxfm.de/"},
  {"FluxFM 70er", "http://streams.fluxfm.de/70er/mp3-320/streams.fluxfm.de/"},
  {"FluxFM 60er", "http://streams.fluxfm.de/60er/mp3-320/streams.fluxfm.de/"},
  {"NRW Schlagerradio", "http://rnrw.cast.addradio.de/rnrw-0182/deinschlager/low/stream.mp3"},
  {"NRW Rockradio", "http://rnrw.cast.addradio.de/rnrw-0182/deinrock/low/stream.mp3"},
  {"NRW 90er", "http://rnrw.cast.addradio.de/rnrw-0182/dein90er/low/stream.mp3"},
  {"Cork 96fm", "https://wg.cdn.tibus.net/96fm"},
  {"TopBlues (US)", "http://us3.internet-radio.com:8342/stream"},
  {"McSlons Irish Pub Radio", "http://server9.streamserver24.com/stream/macslonsradio/stream"}
};

#endif